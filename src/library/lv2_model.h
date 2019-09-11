/**
 * @Brief Wrapper for LV2 plugins - models.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_LV2_MODEL_H
#define SUSHI_LV2_MODEL_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>
#include <mutex>

// Temporary - just to check that it finds them.
#include <lilv-0/lilv/lilv.h>

#include "lv2/resize-port/resize-port.h"
#include "lv2/midi/midi.h"
#include "lv2/log/log.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/data-access/data-access.h"
#include "lv2/options/options.h"
#include "lv2/parameters/parameters.h"
#include "lv2/patch/patch.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2/port-props/port-props.h"
#include "lv2/presets/presets.h"
#include "lv2/state/state.h"
#include "lv2/time/time.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"

#include "processor.h"

#include "lv2_symap.h"

#include "../engine/base_event_dispatcher.h"
#include "lv2_evbuf.h"

namespace sushi {
namespace lv2 {

// From LV2Model example:
/* Size factor for UI ring buffers.  The ring size is a few times the size of
   an event output to give the UI a chance to keep up. Experiments with Ingen,
   which can highly saturate its event output, led me to this value. It
   really ought to be enough for anybody(TM).
*/
#define N_BUFFER_CYCLES 16

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

class LV2Model;

/** Type of plugin control. */
typedef enum
{
    PORT, ///< Control port
    PROPERTY ///< Property (set via atom message)
} ControlType;

typedef struct
{
    float value;
    char* label;
} ScalePoint;

/** Order scale points by value. */
int scale_point_cmp(const ScalePoint* a, const ScalePoint* b);

/** Plugin control. */
typedef struct
{
    LV2Model* model; // TODO: Is this needed?
    ControlType type;
    LilvNode* node;
    LilvNode* symbol; ///< Symbol
    LilvNode* label; ///< Human readable label
    LV2_URID property; ///< Iff type == PROPERTY
    int index; ///< Iff type == PORT
    LilvNode* group; ///< Port/control group, or NULL
//  void* widget; ///< Control Widget
    int n_points; ///< Number of scale points
    ScalePoint* points; ///< Scale points
    LV2_URID value_type; ///< Type of control value
    LilvNode* min; ///< Minimum value
    LilvNode* max; ///< Maximum value
    LilvNode* def; ///< Default value
    bool is_toggle; ///< Boolean (0 and 1 only)
    bool is_integer; ///< Integer values only
    bool is_enumeration; ///< Point values only
    bool is_logarithmic; ///< Logarithmic scale
    bool is_writable; ///< Writable (input)
    bool is_readable; ///< Readable (output)
} ControlID;

ControlID* new_port_control(LV2Model* model, uint32_t index);

ControlID* new_property_control(LV2Model* model, const LilvNode* property);

typedef struct
{
    size_t n_controls{0};
    ControlID** controls{nullptr};
} Controls;

void add_control(Controls* controls, ControlID* control);

ControlID* get_property_control(const Controls* controls, LV2_URID property);

/**
Control change event, sent through ring buffers for UI updates.
*/
typedef struct
{
    uint32_t index;
    uint32_t protocol;
    uint32_t size;
    uint8_t  body[];
} ControlChange;

enum PortFlow
{
    FLOW_UNKNOWN,
    FLOW_INPUT,
    FLOW_OUTPUT
};

enum PortType
{
    TYPE_UNKNOWN,
    TYPE_CONTROL,
    TYPE_AUDIO,
    TYPE_EVENT,
    TYPE_CV
};

class Port
{
public:
    const LilvPort* lilv_port; ///< LV2 port
    enum PortType type; ///< Data type
    enum PortFlow flow; ///< Data flow direction

    LV2_Evbuf* evbuf; ///< For MIDI ports, otherwise NULL

    void* widget; ///< Control widget, if applicable
    int buf_size; ///< Custom buffer size, or 0
    int index; ///< Port index
    float control; ///< For control ports, otherwise 0.0f

    // For ranges. Only used in control ports.
    float def{1.0f};
    float max{1.0f};
    float min{0.0f};
};

typedef struct
{
    LV2_URID atom_Float;
    LV2_URID atom_Int;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_String;
    LV2_URID atom_eventTransfer;
    LV2_URID bufsz_maxBlockLength;
    LV2_URID bufsz_minBlockLength;
    LV2_URID bufsz_sequenceSize;
    LV2_URID log_Error;
    LV2_URID log_Trace;
    LV2_URID log_Warning;
    LV2_URID midi_MidiEvent;
    LV2_URID param_sampleRate;
    LV2_URID patch_Get;
    LV2_URID patch_Put;
    LV2_URID patch_Set;
    LV2_URID patch_body;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID time_Position;
    LV2_URID time_bar;
    LV2_URID time_barBeat;
    LV2_URID time_beatUnit;
    LV2_URID time_beatsPerBar;
    LV2_URID time_beatsPerMinute;
    LV2_URID time_frame;
    LV2_URID time_speed;
    LV2_URID ui_updateRate;
} LV2_URIDs;

// Ilias TODO: Unsure if these are global, or per plugin, yet.
class Lv2_Host_Nodes
{
public:
    Lv2_Host_Nodes(LilvWorld* world)
    {
        /* Cache URIs for concepts we'll use */
        this->atom_AtomPort = lilv_new_uri(world, LV2_ATOM__AtomPort);
        this->atom_Chunk = lilv_new_uri(world, LV2_ATOM__Chunk);
        this->atom_Float = lilv_new_uri(world, LV2_ATOM__Float);
        this->atom_Path = lilv_new_uri(world, LV2_ATOM__Path);
        this->atom_Sequence = lilv_new_uri(world, LV2_ATOM__Sequence);
        this->lv2_AudioPort = lilv_new_uri(world, LV2_CORE__AudioPort);
        this->lv2_CVPort = lilv_new_uri(world, LV2_CORE__CVPort);
        this->lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
        this->lv2_InputPort = lilv_new_uri(world, LV2_CORE__InputPort);
        this->lv2_OutputPort = lilv_new_uri(world, LV2_CORE__OutputPort);
        this->lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
        this->lv2_control = lilv_new_uri(world, LV2_CORE__control);
        this->lv2_default = lilv_new_uri(world, LV2_CORE__default);
        this->lv2_enumeration = lilv_new_uri(world, LV2_CORE__enumeration);
        this->lv2_integer = lilv_new_uri(world, LV2_CORE__integer);
        this->lv2_maximum = lilv_new_uri(world, LV2_CORE__maximum);
        this->lv2_minimum = lilv_new_uri(world, LV2_CORE__minimum);
        this->lv2_name = lilv_new_uri(world, LV2_CORE__name);
        this->lv2_reportsLatency = lilv_new_uri(world, LV2_CORE__reportsLatency);
        this->lv2_sampleRate = lilv_new_uri(world, LV2_CORE__sampleRate);
        this->lv2_symbol = lilv_new_uri(world, LV2_CORE__symbol);
        this->lv2_toggled = lilv_new_uri(world, LV2_CORE__toggled);
        this->midi_MidiEvent = lilv_new_uri(world, LV2_MIDI__MidiEvent);
        this->pg_group = lilv_new_uri(world, LV2_PORT_GROUPS__group);
        this->pprops_logarithmic = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
        this->pprops_notOnGUI = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
        this->pprops_rangeSteps = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
        this->pset_Preset = lilv_new_uri(world, LV2_PRESETS__Preset);
        this->pset_bank = lilv_new_uri(world, LV2_PRESETS__bank);
        this->rdfs_comment = lilv_new_uri(world, LILV_NS_RDFS "comment");
        this->rdfs_label = lilv_new_uri(world, LILV_NS_RDFS "label");
        this->rdfs_range = lilv_new_uri(world, LILV_NS_RDFS "range");
        this->rsz_minimumSize = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
        this->work_interface = lilv_new_uri(world, LV2_WORKER__interface);
        this->work_schedule = lilv_new_uri(world, LV2_WORKER__schedule);
    }

    ~Lv2_Host_Nodes()
    {
        lilv_node_free(atom_AtomPort);
        lilv_node_free(atom_Chunk);
        lilv_node_free(atom_Float);
        lilv_node_free(atom_Path);
        lilv_node_free(atom_Sequence);

        lilv_node_free(lv2_AudioPort);
        lilv_node_free(lv2_CVPort);
        lilv_node_free(lv2_ControlPort);
        lilv_node_free(lv2_InputPort);
        lilv_node_free(lv2_OutputPort);
        lilv_node_free(lv2_connectionOptional);
        lilv_node_free(lv2_control);
        lilv_node_free(lv2_default);
        lilv_node_free(lv2_enumeration);
        lilv_node_free(lv2_integer);
        lilv_node_free(lv2_maximum);
        lilv_node_free(lv2_minimum);
        lilv_node_free(lv2_name);
        lilv_node_free(lv2_reportsLatency);
        lilv_node_free(lv2_sampleRate);
        lilv_node_free(lv2_symbol);
        lilv_node_free(lv2_toggled);
        lilv_node_free(midi_MidiEvent);
        lilv_node_free(pg_group);
        lilv_node_free(pprops_logarithmic);
        lilv_node_free(pprops_notOnGUI);
        lilv_node_free(pprops_rangeSteps);
        lilv_node_free(pset_Preset);
        lilv_node_free(pset_bank);
        lilv_node_free(rdfs_comment);
        lilv_node_free(rdfs_label);
        lilv_node_free(rdfs_range);
        lilv_node_free(rsz_minimumSize);
        lilv_node_free(work_interface);
        lilv_node_free(work_schedule);
    }

    LilvNode* atom_AtomPort;
    LilvNode* atom_Chunk;
    LilvNode* atom_Float;
    LilvNode* atom_Path;
    LilvNode* atom_Sequence;

    LilvNode* lv2_AudioPort;
    LilvNode* lv2_CVPort;
    LilvNode* lv2_ControlPort;
    LilvNode* lv2_InputPort;
    LilvNode* lv2_OutputPort;
    LilvNode* lv2_connectionOptional;
    LilvNode* lv2_control;
    LilvNode* lv2_default;
    LilvNode* lv2_enumeration;
    LilvNode* lv2_integer;
    LilvNode* lv2_maximum;
    LilvNode* lv2_minimum;
    LilvNode* lv2_name;
    LilvNode* lv2_reportsLatency;
    LilvNode* lv2_sampleRate;
    LilvNode* lv2_symbol;
    LilvNode* lv2_toggled;
    LilvNode* midi_MidiEvent;
    LilvNode* pg_group;
    LilvNode* pprops_logarithmic;
    LilvNode* pprops_notOnGUI;
    LilvNode* pprops_rangeSteps;
    LilvNode* pset_Preset;
    LilvNode* pset_bank;
    LilvNode* rdfs_comment;
    LilvNode* rdfs_label;
    LilvNode* rdfs_range;
    LilvNode* rsz_minimumSize;
    LilvNode* work_interface;
    LilvNode* work_schedule;
};

typedef enum
{
    LV2_RUNNING,
    LV2_PAUSE_REQUESTED,
    LV2_PAUSED
} Lv2_PlayState;

typedef struct {
    LV2Model* model; // TODO: Is this needed?

    // ZixRing* requests; ///< Requests to the worker
    // ZixRing* responses; ///< Responses from the worker

// TODO: Introduce proper thread. std::thread
//  ZixThread thread; ///< Worker thread

    void* response; ///< Worker response buffer
    std::mutex sem; ///< Worker semaphore

    const LV2_Worker_Interface* iface; ///< Plugin worker interface
    bool threaded; ///< Run work in another thread
} Lv2_Worker;

typedef struct
{
    LV2_Feature map_feature;
    LV2_Feature unmap_feature;
    LV2_State_Make_Path make_path;
    LV2_Feature make_path_feature;
    LV2_Worker_Schedule sched;
    LV2_Feature sched_feature;
    LV2_Worker_Schedule ssched;
    LV2_Feature state_sched_feature;
    LV2_Log_Log llog;
    LV2_Feature log_feature;
    LV2_Options_Option options[6];
    LV2_Feature options_feature;
    LV2_Feature safe_restore_feature;
    LV2_Extension_Data_Feature ext_data;
} Lv2_Host_Features;

class LV2Model
{
public:
    LV2Model(LilvWorld* worldIn):
    world(worldIn),
    nodes(worldIn)
    {
        // This allows loading plu-ins from their URI's, assuming they are installed in the correct paths
        // on the local machine.
        /* Find all installed plugins */
        lilv_world_load_all(world);

        _initialize_map_feature();

        //_model.worker.model       = _model;
        //_model.state_worker.model = _model;

        _initialize_unmap_feature();
        _initialize_urid_symap();
        _initialize_log_feature();
    }

    ~LV2Model()
    {

    }

    LV2_URIDs urids;  ///< URIDs
    Lv2_Host_Nodes nodes; ///< Nodes

    LV2_Atom_Forge forge; ///< Atom forge

    LilvWorld* world; ///< Lilv World

    LV2_URID_Map  map; ///< URI => Int map
    LV2_URID_Unmap unmap; ///< Int => URI map

    Symap* symap; ///< URI map
    std::mutex symap_lock; ///< Lock for URI map

    const LilvPlugin* plugin; ///< Plugin class (RDF data)
    LilvState* preset;  ///< Current preset

    LilvInstance* instance{nullptr}; ///< Plugin instance (shared library)

    struct Port* ports; ///< Port array of size num_ports

    int midi_buf_size{4096}; ///< Size of MIDI port buffers

    int control_in; ///< Index of control input port

    int num_ports; ///< Size of the two following arrays:

    int plugin_latency{0}; ///< Latency reported by plugin (if any)

    float sample_rate; ///< Sample rate

    bool buf_size_set{false}; ///< True iff buffer size callback fired

    bool exit; ///< True iff execution is finished

    bool request_update{false}; ///< True iff a plugin update is needed

    Lv2_Worker worker; ///< Worker thread implementation
    Lv2_Worker state_worker; ///< Synchronous worker for state restore
    std::mutex work_lock; ///< Lock for plugin work() method
    std::mutex done; ///< Exit semaphore
    std::mutex paused; ///< Paused signal from process thread
    Lv2_PlayState play_state; ///< Current play state

/*
    char*              temp_dir;       ///< Temporary plugin state directory
    char*              save_dir;       ///< Plugin save directory
*/
//  bool               safe_restore;   ///< Plugin restore() is thread-safe

// TODO: Ilias The below needs re-introducing for control no?
    bool               has_ui;         ///< True iff a control UI is present
    Controls           controls;       ///< Available plugin controls
//  uint32_t           event_delta_t;  ///< Frames since last update sent to UI
//  float              ui_update_hz;   ///< Frequency of UI updates

// void* window; ///< Window (if applicable)

    LilvUIs*           uis;            ///< All plugin UIs (RDF data)LilvInstance
    const LilvUI*      ui;             ///< Plugin UI (RDF data)
    const LilvNode*    ui_type;        ///< Plugin UI type (unwrapped)

//  SerdEnv*           env;            ///< Environment for RDF printing

//  TODO: This is a separate library. Only used once for logging, I could include that later.
//  Sratom*            sratom;         ///< Atom serialiser
//  Sratom*            ui_sratom;      ///< Atom serialiser for UI thread

// I either include this, or use a different ringbuffer already used in Sushi.
//  ZixRing*           ui_events;      ///< Port events from UI
//  ZixRing*           plugin_events;  ///< Port events from plugin

//  void*              ui_event_buf;   ///< Buffer for reading UI port events

//  uint32_t           position;       ///< Transport position in frames
//  float              bpm;            ///< Transport tempo in beats per minute
//  bool               rolling;        ///< Transport speed (0=stop, 1=play)

    Lv2_Host_Features   _features;
    const LV2_Feature** feature_list;

    bool initialize_host_feature_list();

private:
    void _initialize_map_feature();
    void _initialize_unmap_feature();
    void _initialize_log_feature();
    void _initialize_urid_symap();
};

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_MODEL_H