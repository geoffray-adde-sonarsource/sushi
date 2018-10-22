#ifndef SUSHI_PERFORMANCE_TIMER_H
#define SUSHI_PERFORMANCE_TIMER_H

#include <chrono>
#include <optional>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "twine/twine.h"

#include "constants.h"
#include "spinlock.h"

namespace sushi {
namespace performance {

using TimePoint = std::chrono::nanoseconds;
constexpr int MAX_LOG_ENTRIES = 20000;

struct ProcessTimings
{
    ProcessTimings() : avg_case{0.0f}, min_case{100.0f}, max_case{0.0f} {}
    ProcessTimings(float avg, float min, float max) : avg_case{avg}, min_case{min}, max_case{max} {}
    float avg_case{1};
    float min_case{1};
    float max_case{0};
};

class PerformanceTimer
{
public:
    MIND_DECLARE_NON_COPYABLE(PerformanceTimer);

    PerformanceTimer() = default;
    ~PerformanceTimer();

    /**
     * @brief Set the period to use for timings
     * @param timing_period The timing period in nanoseconds
     */
    void set_timing_period(TimePoint timing_period);

    /**
     * @brief Ser the period to use for timings implicitly
     * @param samplerate The samplerate in Hz
     * @param buffer_size The audio buffer size in samples
     */
    void set_timing_period(float samplerate, int buffer_size);

    /**
     * @brief Entry point for timing section
     * @return A timestamp representing the start of the timing period
     */
    TimePoint start_timer()
    {
        if (_enabled)
        {
            return twine::current_rt_time();
        }
        return std::chrono::nanoseconds(0);
    }

    /**
     * @brief Exit point for timing section.
     * @param start_time A timestamp from a previous call to start_timer()
     * @param node_id An integer id to identify timings from this node
     */
    void stop_timer(TimePoint start_time, int node_id)
    {
        if (_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _entry_queue.push(tp);
            // if queue is full, drop entries silently.
        }
    }

    /**
     * @brief Exit point for timing section. Safe to call concurrently from
     *       several threads.
     * @param start_time A timestamp from a previous call to start_timer()
     * @param node_id An integer id to identify timings from this node
     */
    void stop_timer_rt_safe(TimePoint start_time, int node_id)
    {
        if(_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _queue_lock.lock();
            _entry_queue.push(tp);
            _queue_lock.unlock();
            // if queue is full, drop entries silently.
        }
    }

    /**
     * @brief Enable or disable timings
     * @param enabled Enable timings if true, disable if false
     */
    void enable(bool enabled);

    /**
     * @brief Get the recorded timings from a specific node
     * @param id An integer id representing a timing node
     * @return A ProcessTimings object populated if the node has any timing records. Empty otherwise
     */
    std::optional<ProcessTimings> timings_for_node(int id);

    /**
     * @brief Clear the recorded timings for a particular node
     * @param id An integer id representing a timing node
     * @return true if the node was found, false otherwise
     */
    bool clear_timings_for_node(int id);

    /**
     * @brief Reset all recorded timings
     */
    void clear_all_timings();

protected:

    struct TimingLogPoint
    {
        int id;
        TimePoint delta_time;
    };

    struct TimingNode
    {
        int id;
        ProcessTimings timings;
    };

    void _worker();
    void _update_timings();

    ProcessTimings _calculate_timings(const std::vector<TimingLogPoint>& entries);
    ProcessTimings _merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings);

    std::thread _process_thread;
    float _period;
    std::atomic_bool _enabled;

    std::map<int, TimingNode>  _timings;
    std::mutex _timing_lock;
    SpinLock _queue_lock;
    alignas(ASSUMED_CACHE_LINE_SIZE) memory_relaxed_aquire_release::CircularFifo<TimingLogPoint, MAX_LOG_ENTRIES> _entry_queue;
};

} // namespace performance
} // namespace sushi

#endif //SUSHI_PERFORMANCE_TIMER_H
