/*
 * Minimal CLAP test stub - Synth with parameters
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "clap/clap.h"

/* Plugin state */
typedef struct plugin_data {
    double cutoff;
    double resonance;
    double volume;
} plugin_data_t;

/* Parameter IDs */
enum {
    PARAM_CUTOFF = 0,
    PARAM_RESONANCE = 1,
    PARAM_VOLUME = 2,
    PARAM_COUNT = 3
};

/* Plugin descriptor */
static const char *features[] = { CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, NULL };

static const clap_plugin_descriptor_t s_desc = {
    .clap_version = CLAP_VERSION,
    .id = "test.param",
    .name = "Test Param Synth",
    .vendor = "Test",
    .url = "",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "Test synth with parameters",
    .features = features
};

/* Params extension */
static uint32_t params_count(const clap_plugin_t *plugin) {
    return PARAM_COUNT;
}

static bool params_get_info(const clap_plugin_t *plugin, uint32_t index, clap_param_info_t *info) {
    switch (index) {
        case PARAM_CUTOFF:
            info->id = PARAM_CUTOFF;
            strncpy(info->name, "Cutoff", CLAP_NAME_SIZE);
            strncpy(info->module, "", CLAP_PATH_SIZE);
            info->min_value = 20.0;
            info->max_value = 20000.0;
            info->default_value = 1000.0;
            info->flags = CLAP_PARAM_IS_AUTOMATABLE;
            info->cookie = NULL;
            return true;
        case PARAM_RESONANCE:
            info->id = PARAM_RESONANCE;
            strncpy(info->name, "Resonance", CLAP_NAME_SIZE);
            strncpy(info->module, "", CLAP_PATH_SIZE);
            info->min_value = 0.0;
            info->max_value = 1.0;
            info->default_value = 0.0;
            info->flags = CLAP_PARAM_IS_AUTOMATABLE;
            info->cookie = NULL;
            return true;
        case PARAM_VOLUME:
            info->id = PARAM_VOLUME;
            strncpy(info->name, "Volume", CLAP_NAME_SIZE);
            strncpy(info->module, "", CLAP_PATH_SIZE);
            info->min_value = 0.0;
            info->max_value = 1.0;
            info->default_value = 0.8;
            info->flags = CLAP_PARAM_IS_AUTOMATABLE;
            info->cookie = NULL;
            return true;
    }
    return false;
}

static bool params_get_value(const clap_plugin_t *plugin, clap_id id, double *value) {
    plugin_data_t *data = (plugin_data_t *)plugin->plugin_data;
    switch (id) {
        case PARAM_CUTOFF: *value = data->cutoff; return true;
        case PARAM_RESONANCE: *value = data->resonance; return true;
        case PARAM_VOLUME: *value = data->volume; return true;
    }
    return false;
}

static bool params_value_to_text(const clap_plugin_t *plugin, clap_id id, double value, char *display, uint32_t size) {
    snprintf(display, size, "%.2f", value);
    return true;
}

static bool params_text_to_value(const clap_plugin_t *plugin, clap_id id, const char *text, double *value) {
    *value = atof(text);
    return true;
}

static void params_flush(const clap_plugin_t *plugin, const clap_input_events_t *in, const clap_output_events_t *out) {
    /* Process parameter change events */
}

static const clap_plugin_params_t s_params = {
    .count = params_count,
    .get_info = params_get_info,
    .get_value = params_get_value,
    .value_to_text = params_value_to_text,
    .text_to_value = params_text_to_value,
    .flush = params_flush
};

/* Audio ports extension - output only (synth) */
static uint32_t audio_ports_count(const clap_plugin_t *plugin, bool is_input) {
    return is_input ? 0 : 1;
}

static bool audio_ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input, clap_audio_port_info_t *info) {
    if (is_input || index != 0) return false;
    info->id = 0;
    strncpy(info->name, "Output", CLAP_NAME_SIZE);
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

static const clap_plugin_audio_ports_t s_audio_ports = {
    .count = audio_ports_count,
    .get = audio_ports_get
};

/* Note ports extension - MIDI input */
static uint32_t note_ports_count(const clap_plugin_t *plugin, bool is_input) {
    return is_input ? 1 : 0;
}

static bool note_ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input, clap_note_port_info_t *info) {
    if (!is_input || index != 0) return false;
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    strncpy(info->name, "MIDI In", CLAP_NAME_SIZE);
    return true;
}

static const clap_plugin_note_ports_t s_note_ports = {
    .count = note_ports_count,
    .get = note_ports_get
};

/* Plugin methods */
static bool plugin_init(const clap_plugin_t *plugin) {
    plugin_data_t *data = (plugin_data_t *)plugin->plugin_data;
    data->cutoff = 1000.0;
    data->resonance = 0.0;
    data->volume = 0.8;
    return true;
}

static void plugin_destroy(const clap_plugin_t *plugin) {
    free(plugin->plugin_data);
    free((void*)plugin);
}

static bool plugin_activate(const clap_plugin_t *plugin, double sr, uint32_t min, uint32_t max) { return true; }
static void plugin_deactivate(const clap_plugin_t *plugin) {}
static bool plugin_start_processing(const clap_plugin_t *plugin) { return true; }
static void plugin_stop_processing(const clap_plugin_t *plugin) {}
static void plugin_reset(const clap_plugin_t *plugin) {}

static clap_process_status plugin_process(const clap_plugin_t *plugin, const clap_process_t *process) {
    /* Silence output */
    for (uint32_t c = 0; c < process->audio_outputs[0].channel_count; c++) {
        memset(process->audio_outputs[0].data32[c], 0, process->frames_count * sizeof(float));
    }
    return CLAP_PROCESS_CONTINUE;
}

static const void *plugin_get_extension(const clap_plugin_t *plugin, const char *id) {
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &s_audio_ports;
    if (!strcmp(id, CLAP_EXT_NOTE_PORTS)) return &s_note_ports;
    if (!strcmp(id, CLAP_EXT_PARAMS)) return &s_params;
    return NULL;
}

static void plugin_on_main_thread(const clap_plugin_t *plugin) {}

/* Factory */
static uint32_t factory_get_plugin_count(const clap_plugin_factory_t *factory) { return 1; }

static const clap_plugin_descriptor_t *factory_get_plugin_descriptor(const clap_plugin_factory_t *factory, uint32_t index) {
    return index == 0 ? &s_desc : NULL;
}

static const clap_plugin_t *factory_create_plugin(const clap_plugin_factory_t *factory, const clap_host_t *host, const char *plugin_id) {
    if (strcmp(plugin_id, s_desc.id)) return NULL;

    clap_plugin_t *p = (clap_plugin_t*)calloc(1, sizeof(clap_plugin_t));
    p->desc = &s_desc;
    p->plugin_data = calloc(1, sizeof(plugin_data_t));
    p->init = plugin_init;
    p->destroy = plugin_destroy;
    p->activate = plugin_activate;
    p->deactivate = plugin_deactivate;
    p->start_processing = plugin_start_processing;
    p->stop_processing = plugin_stop_processing;
    p->reset = plugin_reset;
    p->process = plugin_process;
    p->get_extension = plugin_get_extension;
    p->on_main_thread = plugin_on_main_thread;
    return p;
}

static const clap_plugin_factory_t s_factory = {
    .get_plugin_count = factory_get_plugin_count,
    .get_plugin_descriptor = factory_get_plugin_descriptor,
    .create_plugin = factory_create_plugin
};

/* Entry point */
static bool entry_init(const char *path) { return true; }
static void entry_deinit(void) {}
static const void *entry_get_factory(const char *factory_id) {
    return !strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) ? &s_factory : NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory
};
