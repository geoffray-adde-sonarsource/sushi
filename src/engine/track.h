/**
 * @Brief Class to represent a mixer track with a chain of processors
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_TRACK_H
#define SUSHI_TRACK_H

#include <string>
#include <memory>
#include <array>
#include <vector>

#include "library/sample_buffer.h"
#include "library/internal_plugin.h"
#include "library/rt_event_fifo.h"
#include "library/constants.h"
#include "library/performance_timer.h"

namespace sushi {
namespace engine {

/* No real technical limit, just something arbitrarily high enough */
constexpr int TRACK_MAX_CHANNELS = 10;
constexpr int TRACK_MAX_BUSSES = TRACK_MAX_CHANNELS / 2;

class Track : public InternalPlugin, public RtEventPipe
{
public:
    MIND_DECLARE_NON_COPYABLE(Track);

    /**
     * @brief Create a track with a given number of channels
     * @param channels The number of channels in the track.
     *                 Note that even mono tracks have a stereo output bus
     */
    Track(HostControl host_control, int channels, performance::PerformanceTimer* timer);

    /**
     * @brief Create a track with a given number of stereo input and output busses
     *        Busses are an abstraction for busses*2 channels internally.
     * @param input_buffers The number of input busses
     * @param output_buffers The number of output busses
     */
    Track(HostControl host_control, int input_busses, int output_busses, performance::PerformanceTimer* timer);

    ~Track() = default;

    /**
     * @brief Adds a plugin to the end of the track.
     * @param The plugin to add.
     */
    bool add(Processor* processor);

    /**
     * @brief Remove a plugin from the track.
     * @param processor The ObjectId of the processor to remove
     * @return true if the processor was found and succesfully removed, false otherwise
     */
    bool remove(ObjectId processor);

    /**
     * @brief Return a SampleBuffer to an input bus
     * @param bus The index of the bus, must not be greater than the number of busses configured
     * @return A non-owning SampleBuffer pointing to the given bus
     */
    ChunkSampleBuffer input_bus(int bus)
    {
        assert(bus < _input_busses);
        return ChunkSampleBuffer::create_non_owning_buffer(_input_buffer, bus * 2, 2);
    }

    /**
    * @brief Return a SampleBuffer to an output bus
    * @param bus The index of the bus, must not be greater than the number of busses configured
    * @return A non-owning SampleBuffer pointing to the given bus
    */
    ChunkSampleBuffer output_bus(int bus)
    {
        assert(bus < _output_busses);
        return ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, bus * 2, 2);
    }

    /**
     * @brief Return a reference to an RtEventFifo containing RtEvents outputed from the
     *        processors on the track. set_event_output_internal() must be called first
     *        to direct outputed event to the internal buffer.
     *
     * @return A reference to an RtEventFifo containing buffered events
     */
    RtEventFifo& output_event_buffer()
    {
        return _output_event_buffer;
    }

    /**
     * @brief If called, events from processors will be buffered internally in a queue
     *        instead of being passed on to the set event output. Events can then be
     *        retrieved by calling output_event_buffer() to access the buffer.
     *        This is useful in multithreaded processing where multiple tracks might
     *        otherwise output to the same event output.
     */
    void set_event_output_internal()
    {
        set_event_output(&_output_event_buffer);
    }

    /**
     * @brief Return a SampleBuffer to an input channel
     * @param bus The index of the channel, must not be greater than the number of channels configured
     * @return A non-owning SampleBuffer pointing to the given bus
     */
    ChunkSampleBuffer input_channel(int index)
    {
        assert(index < _max_input_channels);
        return ChunkSampleBuffer::create_non_owning_buffer(_input_buffer, index, 1);
    }

    /**
    * @brief Return a SampleBuffer to an output bus
    * @param bus The index of the channel, must not be greater than the number of channels configured
    * @return A non-owning SampleBuffer pointing to the given bus
    */
    ChunkSampleBuffer output_channel(int index)
    {
        assert(index < _max_output_channels);
        return ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, index, 1);
    }

    /**
     * @brief Return the number of input busses of the track.
     * @return The number of input busses on the track.
     */
    int input_busses()
    {
        return _input_busses;
    }

    /**
     * @brief Return the number of input busses of the track.
     * @return The number of input busses on the track.
     */
    int output_busses()
    {
        return _output_busses;
    }

    /**
     * @brief Render all processors of the track. Should be called after process_event() and
     *        after input buffers have been filled
     */
    void render();

    /**
     * @brief Static render function for passing to a thread manager
     * @param arg Void* pointing to an instance of a Track.
     */
    static void ext_render_function(void* arg)
    {
        reinterpret_cast<Track*>(arg)->render();
    }

    /* Inherited from Processor */
    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out) override;

    void set_bypassed(bool bypassed) override;

    void set_input_channels(int channels) override
    {
        Processor::set_input_channels(channels);
        _update_channel_config();
    }

    void set_output_channels(int channels) override
    {
        Processor::set_output_channels(channels);
        _update_channel_config();
    }

    const std::vector<Processor*> process_chain()
    {
        return _processors;
    }

    /* Inherited from RtEventPipe */
    void send_event(RtEvent event) override;

private:
    void _common_init();
    void _update_channel_config();
    void _process_output_events();

    std::vector<Processor*> _processors;
    ChunkSampleBuffer _input_buffer;
    ChunkSampleBuffer _output_buffer;

    int _input_busses;
    int _output_busses;
    bool _multibus;

    std::array<FloatParameterValue*, TRACK_MAX_BUSSES> _gain_parameters;
    std::array<FloatParameterValue*, TRACK_MAX_BUSSES> _pan_parameters;

    performance::PerformanceTimer* _timer;

    RtEventFifo _kb_event_buffer;
    RtEventFifo _output_event_buffer;
};

void apply_pan_and_gain(ChunkSampleBuffer& buffer, float gain, float pan);


} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
