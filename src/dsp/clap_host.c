/*
 * CLAP Host Core - Plugin discovery, loading, and processing
 */
#include "clap_host.h"
#include "clap/clap.h"
#include "clap/factory/plugin-factory.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/note-ports.h"
#include "clap/ext/params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>

/* Sample rate for activation */
#define HOST_SAMPLE_RATE 44100.0
#define HOST_MIN_FRAMES 1
#define HOST_MAX_FRAMES 4096

/* Host callbacks (minimal implementation) */
static void host_log(const clap_host_t *host, clap_log_severity severity, const char *msg) {
    fprintf(stderr, "[CLAP] %s\n", msg);
}

static const clap_host_log_t s_host_log = {
    .log = host_log
};

static void host_request_restart(const clap_host_t *host) {}
static void host_request_process(const clap_host_t *host) {}
static void host_request_callback(const clap_host_t *host) {}

static const void *host_get_extension(const clap_host_t *host, const char *extension_id) {
    if (!strcmp(extension_id, CLAP_EXT_LOG)) return &s_host_log;
    return NULL;
}

static const clap_host_t s_host = {
    .clap_version = CLAP_VERSION,
    .host_data = NULL,
    .name = "Move Anything CLAP Host",
    .vendor = "Move Anything",
    .url = "",
    .version = "1.0.0",
    .get_extension = host_get_extension,
    .request_restart = host_request_restart,
    .request_process = host_request_process,
    .request_callback = host_request_callback
};

/* Helper: check if string ends with suffix */
static int ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Helper: add plugin to list */
static int list_add(clap_host_list_t *list, const clap_plugin_info_t *info) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity == 0 ? 16 : list->capacity * 2;
        if (new_cap > CLAP_HOST_MAX_PLUGINS) new_cap = CLAP_HOST_MAX_PLUGINS;
        if (list->count >= new_cap) return -1;

        clap_plugin_info_t *new_items = realloc(list->items, new_cap * sizeof(clap_plugin_info_t));
        if (!new_items) return -1;
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = *info;
    return 0;
}

/* Scan a single .clap file and add plugins to list */
static int scan_clap_file(const char *path, clap_host_list_t *list) {
    void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "[CLAP] dlopen failed for %s: %s\n", path, dlerror());
        return -1;
    }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(handle, "clap_entry");
    if (!entry) {
        fprintf(stderr, "[CLAP] No clap_entry in %s\n", path);
        dlclose(handle);
        return -1;
    }

    /* Initialize entry */
    if (!entry->init(path)) {
        fprintf(stderr, "[CLAP] entry->init failed for %s\n", path);
        dlclose(handle);
        return -1;
    }

    /* Get plugin factory */
    const clap_plugin_factory_t *factory =
        (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!factory) {
        fprintf(stderr, "[CLAP] No plugin factory in %s\n", path);
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    /* Enumerate plugins */
    uint32_t count = factory->get_plugin_count(factory);
    for (uint32_t i = 0; i < count; i++) {
        const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, i);
        if (!desc) continue;

        clap_plugin_info_t info = {0};
        strncpy(info.id, desc->id ? desc->id : "", sizeof(info.id) - 1);
        strncpy(info.name, desc->name ? desc->name : "", sizeof(info.name) - 1);
        strncpy(info.vendor, desc->vendor ? desc->vendor : "", sizeof(info.vendor) - 1);
        strncpy(info.path, path, sizeof(info.path) - 1);
        info.plugin_index = i;

        /* Create temporary instance to query ports */
        const clap_plugin_t *plugin = factory->create_plugin(factory, &s_host, desc->id);
        if (plugin && plugin->init(plugin)) {
            /* Query audio ports */
            const clap_plugin_audio_ports_t *audio_ports =
                (const clap_plugin_audio_ports_t *)plugin->get_extension(plugin, CLAP_EXT_AUDIO_PORTS);
            if (audio_ports) {
                info.has_audio_in = audio_ports->count(plugin, true) > 0;
                info.has_audio_out = audio_ports->count(plugin, false) > 0;
            }

            /* Query note ports */
            const clap_plugin_note_ports_t *note_ports =
                (const clap_plugin_note_ports_t *)plugin->get_extension(plugin, CLAP_EXT_NOTE_PORTS);
            if (note_ports) {
                info.has_midi_in = note_ports->count(plugin, true) > 0;
                info.has_midi_out = note_ports->count(plugin, false) > 0;
            }

            plugin->destroy(plugin);
        }

        list_add(list, &info);
    }

    entry->deinit();
    dlclose(handle);
    return 0;
}

int clap_scan_plugins(const char *dir, clap_host_list_t *out) {
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "[CLAP] Cannot open directory: %s\n", dir);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!ends_with(ent->d_name, ".clap")) continue;

        char path[1280];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        scan_clap_file(path, out);
    }

    closedir(d);
    return 0;
}

void clap_free_plugin_list(clap_host_list_t *list) {
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

int clap_load_plugin(const char *path, int plugin_index, clap_instance_t *out) {
    memset(out, 0, sizeof(*out));

    void *handle = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "[CLAP] dlopen failed: %s\n", dlerror());
        return -1;
    }

    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(handle, "clap_entry");
    if (!entry) {
        fprintf(stderr, "[CLAP] No clap_entry symbol\n");
        dlclose(handle);
        return -1;
    }

    if (!entry->init(path)) {
        fprintf(stderr, "[CLAP] entry->init failed\n");
        dlclose(handle);
        return -1;
    }

    const clap_plugin_factory_t *factory =
        (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!factory) {
        fprintf(stderr, "[CLAP] No plugin factory\n");
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, plugin_index);
    if (!desc) {
        fprintf(stderr, "[CLAP] Invalid plugin index\n");
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    const clap_plugin_t *plugin = factory->create_plugin(factory, &s_host, desc->id);
    if (!plugin) {
        fprintf(stderr, "[CLAP] create_plugin failed\n");
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    if (!plugin->init(plugin)) {
        fprintf(stderr, "[CLAP] plugin->init failed\n");
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    /* Activate the plugin */
    if (!plugin->activate(plugin, HOST_SAMPLE_RATE, HOST_MIN_FRAMES, HOST_MAX_FRAMES)) {
        fprintf(stderr, "[CLAP] plugin->activate failed\n");
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    /* Start processing */
    if (!plugin->start_processing(plugin)) {
        fprintf(stderr, "[CLAP] plugin->start_processing failed\n");
        plugin->deactivate(plugin);
        plugin->destroy(plugin);
        entry->deinit();
        dlclose(handle);
        return -1;
    }

    out->handle = handle;
    out->entry = entry;
    out->factory = factory;
    out->plugin = plugin;
    out->activated = true;
    out->processing = true;
    strncpy(out->path, path, sizeof(out->path) - 1);

    return 0;
}

void clap_unload_plugin(clap_instance_t *inst) {
    if (!inst->plugin) return;

    const clap_plugin_t *plugin = (const clap_plugin_t *)inst->plugin;
    const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)inst->entry;

    if (inst->processing) {
        plugin->stop_processing(plugin);
        inst->processing = false;
    }
    if (inst->activated) {
        plugin->deactivate(plugin);
        inst->activated = false;
    }
    plugin->destroy(plugin);

    if (entry) entry->deinit();
    if (inst->handle) dlclose(inst->handle);

    memset(inst, 0, sizeof(*inst));
}

/* Static buffers for CLAP process (avoid per-call allocation) */
static float *s_in_bufs[2] = {NULL, NULL};
static float *s_out_bufs[2] = {NULL, NULL};
static int s_buf_frames = 0;

/* Empty event list callbacks */
static uint32_t s_empty_size(const clap_input_events_t *list) { return 0; }
static const clap_event_header_t *s_empty_get(const clap_input_events_t *list, uint32_t index) { return NULL; }
static bool s_empty_push(const clap_output_events_t *list, const clap_event_header_t *event) { return true; }

static void ensure_buffers(int frames) {
    if (frames <= s_buf_frames) return;

    for (int i = 0; i < 2; i++) {
        free(s_in_bufs[i]);
        free(s_out_bufs[i]);
        s_in_bufs[i] = calloc(frames, sizeof(float));
        s_out_bufs[i] = calloc(frames, sizeof(float));
    }
    s_buf_frames = frames;
}

int clap_process_block(clap_instance_t *inst, const float *in, float *out, int frames) {
    if (!inst->plugin || !inst->processing) return -1;

    const clap_plugin_t *plugin = (const clap_plugin_t *)inst->plugin;

    ensure_buffers(frames);

    /* De-interleave input if provided */
    if (in) {
        for (int i = 0; i < frames; i++) {
            s_in_bufs[0][i] = in[i * 2];
            s_in_bufs[1][i] = in[i * 2 + 1];
        }
    } else {
        memset(s_in_bufs[0], 0, frames * sizeof(float));
        memset(s_in_bufs[1], 0, frames * sizeof(float));
    }

    /* Clear output buffers */
    memset(s_out_bufs[0], 0, frames * sizeof(float));
    memset(s_out_bufs[1], 0, frames * sizeof(float));

    /* Setup audio buffers */
    clap_audio_buffer_t audio_in = {
        .data32 = s_in_bufs,
        .data64 = NULL,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    clap_audio_buffer_t audio_out = {
        .data32 = s_out_bufs,
        .data64 = NULL,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    /* Empty event lists */
    clap_input_events_t in_events = {
        .ctx = NULL,
        .size = s_empty_size,
        .get = s_empty_get
    };
    clap_output_events_t out_events = {
        .ctx = NULL,
        .try_push = s_empty_push
    };

    /* Setup process struct */
    clap_process_t process = {
        .steady_time = -1,
        .frames_count = frames,
        .transport = NULL,
        .audio_inputs = &audio_in,
        .audio_outputs = &audio_out,
        .audio_inputs_count = 1,
        .audio_outputs_count = 1,
        .in_events = &in_events,
        .out_events = &out_events
    };

    /* Process */
    clap_process_status status = plugin->process(plugin, &process);
    if (status == CLAP_PROCESS_ERROR) {
        return -1;
    }

    /* Interleave output */
    for (int i = 0; i < frames; i++) {
        out[i * 2] = s_out_bufs[0][i];
        out[i * 2 + 1] = s_out_bufs[1][i];
    }

    return 0;
}

int clap_param_count(clap_instance_t *inst) {
    if (!inst->plugin) return 0;

    const clap_plugin_t *plugin = (const clap_plugin_t *)inst->plugin;
    const clap_plugin_params_t *params =
        (const clap_plugin_params_t *)plugin->get_extension(plugin, CLAP_EXT_PARAMS);
    if (!params) return 0;

    return params->count(plugin);
}

int clap_param_info(clap_instance_t *inst, int index, char *name, int name_len, double *min, double *max, double *def) {
    if (!inst->plugin) return -1;

    const clap_plugin_t *plugin = (const clap_plugin_t *)inst->plugin;
    const clap_plugin_params_t *params =
        (const clap_plugin_params_t *)plugin->get_extension(plugin, CLAP_EXT_PARAMS);
    if (!params) return -1;

    clap_param_info_t info;
    if (!params->get_info(plugin, index, &info)) return -1;

    if (name && name_len > 0) {
        strncpy(name, info.name, name_len - 1);
        name[name_len - 1] = '\0';
    }
    if (min) *min = info.min_value;
    if (max) *max = info.max_value;
    if (def) *def = info.default_value;

    return 0;
}

int clap_param_set(clap_instance_t *inst, int index, double value) {
    /* For now, we queue parameter changes via process events
     * This is a simplified implementation - real one would use proper event queuing */
    (void)inst;
    (void)index;
    (void)value;
    return 0;  /* TODO: implement proper parameter setting */
}

double clap_param_get(clap_instance_t *inst, int index) {
    if (!inst->plugin) return 0.0;

    const clap_plugin_t *plugin = (const clap_plugin_t *)inst->plugin;
    const clap_plugin_params_t *params =
        (const clap_plugin_params_t *)plugin->get_extension(plugin, CLAP_EXT_PARAMS);
    if (!params) return 0.0;

    clap_param_info_t info;
    if (!params->get_info(plugin, index, &info)) return 0.0;

    double value = 0.0;
    if (params->get_value(plugin, info.id, &value)) {
        return value;
    }
    return info.default_value;
}

int clap_send_midi(clap_instance_t *inst, const uint8_t *msg, int len) {
    /* TODO: Queue MIDI events for next process call */
    (void)inst;
    (void)msg;
    (void)len;
    return 0;
}
