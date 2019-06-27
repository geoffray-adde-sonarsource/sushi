/**
* Parts taken and/or adapted from:
* MrsWatson - https://github.com/teragonaudio/MrsWatson
*
* Original copyright notice with BSD license:
* Copyright (c) 2013 Teragon Audio. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
*   this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <cstdlib>

#include "library/lv2_plugin_loader.h"
#include "library/lv2_data_structures.h"

#include "library/lv2_host_callback.h"

#include "logging.h"

#include "lv2/atom/atom.h"
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

namespace sushi {
namespace lv2 {

// Ilias TODO: Unsure if these are global, or per plugin, yet.
void populate_nodes(JalvNodes& nodes, LilvWorld* world)
{
    /* Cache URIs for concepts we'll use */
    nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
    nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
    nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
    nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
    nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
    nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
    nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
    nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
    nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
    nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
    nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
    nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
    nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
    nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
    nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
    nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
    nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
    nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
    nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
    nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
    nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
    nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
    nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
    nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
    nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
    nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
    nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
    nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
    nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
    nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
    nodes.rdfs_label             = lilv_new_uri(world, LILV_NS_RDFS "label");
    nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
    nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
    nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
    nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
    nodes.end                    = NULL;
}

void free_nodes(JalvNodes& nodes)
{
    lilv_node_free(nodes.atom_AtomPort);
    lilv_node_free(nodes.atom_Chunk);
    lilv_node_free(nodes.atom_Float);
    lilv_node_free(nodes.atom_Path);
    lilv_node_free(nodes.atom_Sequence);
    lilv_node_free(nodes.lv2_AudioPort);
    lilv_node_free(nodes.lv2_CVPort);
    lilv_node_free(nodes.lv2_ControlPort);
    lilv_node_free(nodes.lv2_InputPort);
    lilv_node_free(nodes.lv2_OutputPort);
    lilv_node_free(nodes.lv2_connectionOptional);
    lilv_node_free(nodes.lv2_control);
    lilv_node_free(nodes.lv2_default);
    lilv_node_free(nodes.lv2_enumeration);
    lilv_node_free(nodes.lv2_integer);
    lilv_node_free(nodes.lv2_maximum);
    lilv_node_free(nodes.lv2_minimum);
    lilv_node_free(nodes.lv2_name);
    lilv_node_free(nodes.lv2_reportsLatency);
    lilv_node_free(nodes.lv2_sampleRate);
    lilv_node_free(nodes.lv2_symbol);
    lilv_node_free(nodes.lv2_toggled);
    lilv_node_free(nodes.midi_MidiEvent);
    lilv_node_free(nodes.pg_group);
    lilv_node_free(nodes.pprops_logarithmic);
    lilv_node_free(nodes.pprops_notOnGUI);
    lilv_node_free(nodes.pprops_rangeSteps);
    lilv_node_free(nodes.pset_Preset);
    lilv_node_free(nodes.pset_bank);
    lilv_node_free(nodes.rdfs_comment);
    lilv_node_free(nodes.rdfs_label);
    lilv_node_free(nodes.rdfs_range);
    lilv_node_free(nodes.rsz_minimumSize);
    lilv_node_free(nodes.work_interface);
    lilv_node_free(nodes.work_schedule);
}

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

// Ilias TODO: Currently allocated plugin instances are not automatically freed when the _loader is destroyed. Should they be?

PluginLoader::PluginLoader()
{
    _world = lilv_world_new();

    // This allows loading plu-ins from their URI's, assuming they are installed in the correct paths
    // on the local machine.
    /* Find all installed plugins */
    lilv_world_load_all(_world);
    //jalv->world = world;

    populate_nodes(_nodes, _world);
}

PluginLoader::~PluginLoader()
{
    free_nodes(_nodes);
    lilv_world_free(_world);
}

LilvInstance* PluginLoader::getPluginInstance()
{
    return _plugin_instance;
}

const LilvPlugin* PluginLoader::get_plugin_handle_from_URI(const std::string &plugin_URI_string)
{
    if (plugin_URI_string.empty())
    {
        MIND_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_world);
    auto plugin_uri = lilv_new_uri(_world, plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        fprintf(stderr, "Missing plugin URI, try lv2ls to list plugins\n");
        // Ilias TODO: Handle error
        return nullptr;
    }

    /* Find plugin */
    printf("Plugin:       %s\n", lilv_node_as_string(plugin_uri));
    const LilvPlugin* plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        fprintf(stderr, "Failed to find plugin\n");

        // Ilias TODO: Handle error

        return nullptr;
    }

    // Ilias TODO: Introduce state_threadSafeRestore later.

    // Ilias TODO: UI code - may not need it - it goes here though.

    return plugin;
}

void PluginLoader::load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list)
{
    /* Instantiate the plugin */
    _plugin_instance = lilv_plugin_instantiate(
            plugin_handle,
            sample_rate,
            feature_list);

    if (_plugin_instance == nullptr)
    {
        fprintf(stderr, "Failed to instantiate plugin.\n");
        // Ilias TODO: Handle error
    }

    // Ilias TODO: Not sure this should be here.
    // Maybe it should be called after ports are "dealt with", whatever that means.
    /* Activate plugin */
    lilv_instance_activate(_plugin_instance);
}

void PluginLoader::close_plugin_instance(LilvInstance *plugin_instance)
{
    if (plugin_instance != nullptr)
    {
        lilv_instance_deactivate(plugin_instance);
        lilv_instance_free(plugin_instance);
    }

    // Ilias TODO: Eventually also free plugin controls one loaded/mapped.

    // Ilias TODO: Terrible design. Either it is a parameter, or it is stored, not both. FIX.
    _plugin_instance = nullptr;
}

} // namespace lv2
} // namespace sushi