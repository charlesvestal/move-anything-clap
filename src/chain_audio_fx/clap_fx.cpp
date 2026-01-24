/*
 * CLAP Audio FX Plugin for Move Anything Signal Chain
 *
 * Allows CLAP effect plugins to be used as audio FX in the chain.
 * MIT License - see LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Inline API definitions to avoid path issues */
extern "C" {
#include <stdint.h>

#define MOVE_PLUGIN_API_VERSION 1
#define MOVE_SAMPLE_RATE 44100
#define MOVE_FRAMES_PER_BLOCK 128

typedef struct host_api_v1 {
    uint32_t api_version;
    int sample_rate;
    int frames_per_block;
    uint8_t *mapped_memory;
    int audio_out_offset;
    int audio_in_offset;
    void (*log)(const char *msg);
    int (*midi_send_internal)(const uint8_t *msg, int len);
    int (*midi_send_external)(const uint8_t *msg, int len);
} host_api_v1_t;

#define AUDIO_FX_API_VERSION 1
#define AUDIO_FX_INIT_SYMBOL "move_audio_fx_init_v1"

typedef struct audio_fx_api_v1 {
    uint32_t api_version;
    int (*on_load)(const char *module_dir, const char *config_json);
    void (*on_unload)(void);
    void (*process_block)(int16_t *audio_inout, int frames);
    void (*set_param)(const char *key, const char *val);
    int (*get_param)(const char *key, char *buf, int buf_len);
} audio_fx_api_v1_t;

#include "dsp/clap_host.h"
}

/* Plugin state */
static const host_api_v1_t *g_host = NULL;
static audio_fx_api_v1_t g_fx_api;

static clap_host_list_t g_plugin_list = {0};
static clap_instance_t g_current_plugin = {0};
static char g_module_dir[256] = "";
static char g_selected_plugin_id[256] = "";

/* Forward declarations */
static void fx_log(const char *msg);

static void fx_log(const char *msg) {
    if (g_host && g_host->log) {
        g_host->log(msg);
    }
    fprintf(stderr, "[CLAP FX] %s\n", msg);
}

/* Find and load a plugin by ID */
static int load_plugin_by_id(const char *plugin_id) {
    /* Scan plugins directory */
    char plugins_dir[512];
    snprintf(plugins_dir, sizeof(plugins_dir), "%s/../clap/plugins", g_module_dir);

    clap_free_plugin_list(&g_plugin_list);
    if (clap_scan_plugins(plugins_dir, &g_plugin_list) != 0) {
        fx_log("Failed to scan plugins directory");
        return -1;
    }

    /* Find plugin by ID */
    for (int i = 0; i < g_plugin_list.count; i++) {
        if (strcmp(g_plugin_list.items[i].id, plugin_id) == 0) {
            /* Found it - must have audio input (be an effect) */
            if (!g_plugin_list.items[i].has_audio_in) {
                fx_log("Plugin is not an audio effect (no audio input)");
                return -1;
            }

            char msg[512];
            snprintf(msg, sizeof(msg), "Loading FX plugin: %s", g_plugin_list.items[i].name);
            fx_log(msg);

            return clap_load_plugin(g_plugin_list.items[i].path,
                                   g_plugin_list.items[i].plugin_index,
                                   &g_current_plugin);
        }
    }

    char msg[512];
    snprintf(msg, sizeof(msg), "Plugin not found: %s", plugin_id);
    fx_log(msg);
    return -1;
}

/* === Audio FX API Implementation === */

static int on_load(const char *module_dir, const char *config_json) {
    fx_log("CLAP FX loading");

    strncpy(g_module_dir, module_dir, sizeof(g_module_dir) - 1);
    g_module_dir[sizeof(g_module_dir) - 1] = '\0';

    /* Parse config JSON for plugin_id if provided */
    if (config_json && strlen(config_json) > 0) {
        /* Simple parsing: look for "plugin_id": "..." */
        const char *id_key = "\"plugin_id\"";
        const char *pos = strstr(config_json, id_key);
        if (pos) {
            pos = strchr(pos, ':');
            if (pos) {
                pos = strchr(pos, '"');
                if (pos) {
                    pos++;
                    const char *end = strchr(pos, '"');
                    if (end) {
                        int len = end - pos;
                        if (len > 0 && len < (int)sizeof(g_selected_plugin_id)) {
                            strncpy(g_selected_plugin_id, pos, len);
                            g_selected_plugin_id[len] = '\0';

                            if (load_plugin_by_id(g_selected_plugin_id) == 0) {
                                fx_log("FX plugin loaded successfully");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

static void on_unload(void) {
    fx_log("CLAP FX unloading");

    if (g_current_plugin.plugin) {
        clap_unload_plugin(&g_current_plugin);
    }
    clap_free_plugin_list(&g_plugin_list);
}

static void process_block(int16_t *audio_inout, int frames) {
    if (!g_current_plugin.plugin) {
        /* No plugin loaded - pass through */
        return;
    }

    /* Convert int16 to float */
    float float_in[MOVE_FRAMES_PER_BLOCK * 2];
    float float_out[MOVE_FRAMES_PER_BLOCK * 2];

    for (int i = 0; i < frames * 2; i++) {
        float_in[i] = audio_inout[i] / 32768.0f;
    }

    /* Process through CLAP plugin */
    if (clap_process_block(&g_current_plugin, float_in, float_out, frames) != 0) {
        /* Error - pass through original */
        return;
    }

    /* Convert float to int16 */
    for (int i = 0; i < frames * 2; i++) {
        float sample = float_out[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        audio_inout[i] = (int16_t)(sample * 32767.0f);
    }
}

static void set_param(const char *key, const char *val) {
    if (!key || !val) return;

    if (strcmp(key, "plugin_id") == 0) {
        if (strcmp(val, g_selected_plugin_id) != 0) {
            /* Unload current */
            if (g_current_plugin.plugin) {
                clap_unload_plugin(&g_current_plugin);
            }

            strncpy(g_selected_plugin_id, val, sizeof(g_selected_plugin_id) - 1);
            g_selected_plugin_id[sizeof(g_selected_plugin_id) - 1] = '\0';

            load_plugin_by_id(g_selected_plugin_id);
        }
    }
    else if (strncmp(key, "param_", 6) == 0) {
        int param_idx = atoi(key + 6);
        double value = atof(val);
        clap_param_set(&g_current_plugin, param_idx, value);
    }
}

static int get_param(const char *key, char *buf, int buf_len) {
    if (!key || !buf || buf_len <= 0) return -1;

    if (strcmp(key, "plugin_id") == 0) {
        return snprintf(buf, buf_len, "%s", g_selected_plugin_id);
    }
    else if (strcmp(key, "plugin_name") == 0) {
        if (g_current_plugin.plugin) {
            /* Find name in list */
            for (int i = 0; i < g_plugin_list.count; i++) {
                if (strcmp(g_plugin_list.items[i].id, g_selected_plugin_id) == 0) {
                    return snprintf(buf, buf_len, "%s", g_plugin_list.items[i].name);
                }
            }
        }
        return snprintf(buf, buf_len, "None");
    }
    else if (strcmp(key, "param_count") == 0) {
        return snprintf(buf, buf_len, "%d", clap_param_count(&g_current_plugin));
    }
    else if (strncmp(key, "param_name_", 11) == 0) {
        int idx = atoi(key + 11);
        char name[64] = "";
        if (clap_param_info(&g_current_plugin, idx, name, sizeof(name), NULL, NULL, NULL) == 0) {
            return snprintf(buf, buf_len, "%s", name);
        }
        return -1;
    }
    else if (strncmp(key, "param_value_", 12) == 0) {
        int idx = atoi(key + 12);
        double value = clap_param_get(&g_current_plugin, idx);
        return snprintf(buf, buf_len, "%.3f", value);
    }

    return -1;
}

/* === Audio FX Entry Point (V1 - kept for compatibility) === */

extern "C" audio_fx_api_v1_t* move_audio_fx_init_v1(const host_api_v1_t *host) {
    g_host = host;

    g_fx_api.api_version = AUDIO_FX_API_VERSION;
    g_fx_api.on_load = on_load;
    g_fx_api.on_unload = on_unload;
    g_fx_api.process_block = process_block;
    g_fx_api.set_param = set_param;
    g_fx_api.get_param = get_param;

    return &g_fx_api;
}

/* ============================================================================
 * Audio FX V2 API - Instance-based for multi-instance support
 * ============================================================================ */

#define AUDIO_FX_API_VERSION_2 2

typedef struct audio_fx_api_v2 {
    uint32_t api_version;
    void* (*create_instance)(const char *module_dir, const char *config_json);
    void (*destroy_instance)(void *instance);
    void (*process_block)(void *instance, int16_t *audio_inout, int frames);
    void (*set_param)(void *instance, const char *key, const char *val);
    int (*get_param)(void *instance, const char *key, char *buf, int buf_len);
} audio_fx_api_v2_t;

/* Per-instance state for V2 API */
typedef struct {
    char module_dir[256];
    char selected_plugin_id[256];
    clap_host_list_t plugin_list;
    clap_instance_t current_plugin;
} clap_fx_instance_t;

static void v2_fx_log(const char *msg) {
    if (g_host && g_host->log) {
        g_host->log(msg);
    }
    fprintf(stderr, "[CLAP FX v2] %s\n", msg);
}

static int v2_load_plugin_by_id(clap_fx_instance_t *inst, const char *plugin_id) {
    char plugins_dir[512];
    snprintf(plugins_dir, sizeof(plugins_dir), "%s/../clap/plugins", inst->module_dir);

    clap_free_plugin_list(&inst->plugin_list);
    if (clap_scan_plugins(plugins_dir, &inst->plugin_list) != 0) {
        v2_fx_log("Failed to scan plugins directory");
        return -1;
    }

    for (int i = 0; i < inst->plugin_list.count; i++) {
        if (strcmp(inst->plugin_list.items[i].id, plugin_id) == 0) {
            if (!inst->plugin_list.items[i].has_audio_in) {
                v2_fx_log("Plugin is not an audio effect (no audio input)");
                return -1;
            }

            char msg[512];
            snprintf(msg, sizeof(msg), "Loading FX plugin: %s", inst->plugin_list.items[i].name);
            v2_fx_log(msg);

            return clap_load_plugin(inst->plugin_list.items[i].path,
                                   inst->plugin_list.items[i].plugin_index,
                                   &inst->current_plugin);
        }
    }

    char msg[512];
    snprintf(msg, sizeof(msg), "Plugin not found: %s", plugin_id);
    v2_fx_log(msg);
    return -1;
}

static void* v2_create_instance(const char *module_dir, const char *config_json) {
    v2_fx_log("Creating CLAP FX instance");

    clap_fx_instance_t *inst = (clap_fx_instance_t*)calloc(1, sizeof(clap_fx_instance_t));
    if (!inst) return NULL;

    strncpy(inst->module_dir, module_dir, sizeof(inst->module_dir) - 1);

    /* Parse config JSON for plugin_id if provided */
    if (config_json && strlen(config_json) > 0) {
        const char *id_key = "\"plugin_id\"";
        const char *pos = strstr(config_json, id_key);
        if (pos) {
            pos = strchr(pos, ':');
            if (pos) {
                pos = strchr(pos, '"');
                if (pos) {
                    pos++;
                    const char *end = strchr(pos, '"');
                    if (end) {
                        int len = end - pos;
                        if (len > 0 && len < (int)sizeof(inst->selected_plugin_id)) {
                            strncpy(inst->selected_plugin_id, pos, len);
                            inst->selected_plugin_id[len] = '\0';
                            v2_load_plugin_by_id(inst, inst->selected_plugin_id);
                        }
                    }
                }
            }
        }
    }

    return inst;
}

static void v2_destroy_instance(void *instance) {
    clap_fx_instance_t *inst = (clap_fx_instance_t*)instance;
    if (!inst) return;

    v2_fx_log("Destroying CLAP FX instance");

    if (inst->current_plugin.plugin) {
        clap_unload_plugin(&inst->current_plugin);
    }
    clap_free_plugin_list(&inst->plugin_list);
    free(inst);
}

static void v2_process_block(void *instance, int16_t *audio_inout, int frames) {
    clap_fx_instance_t *inst = (clap_fx_instance_t*)instance;
    if (!inst || !inst->current_plugin.plugin) {
        return;  /* Pass through */
    }

    float float_in[MOVE_FRAMES_PER_BLOCK * 2];
    float float_out[MOVE_FRAMES_PER_BLOCK * 2];

    for (int i = 0; i < frames * 2; i++) {
        float_in[i] = audio_inout[i] / 32768.0f;
    }

    if (clap_process_block(&inst->current_plugin, float_in, float_out, frames) != 0) {
        return;  /* Error - pass through */
    }

    for (int i = 0; i < frames * 2; i++) {
        float sample = float_out[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        audio_inout[i] = (int16_t)(sample * 32767.0f);
    }
}

static void v2_set_param(void *instance, const char *key, const char *val) {
    clap_fx_instance_t *inst = (clap_fx_instance_t*)instance;
    if (!inst || !key || !val) return;

    if (strcmp(key, "plugin_id") == 0) {
        if (strcmp(val, inst->selected_plugin_id) != 0) {
            if (inst->current_plugin.plugin) {
                clap_unload_plugin(&inst->current_plugin);
            }
            strncpy(inst->selected_plugin_id, val, sizeof(inst->selected_plugin_id) - 1);
            v2_load_plugin_by_id(inst, inst->selected_plugin_id);
        }
    }
    else if (strncmp(key, "param_", 6) == 0) {
        int param_idx = atoi(key + 6);
        double value = atof(val);
        clap_param_set(&inst->current_plugin, param_idx, value);
    }
}

static int v2_get_param(void *instance, const char *key, char *buf, int buf_len) {
    clap_fx_instance_t *inst = (clap_fx_instance_t*)instance;
    if (!inst || !key || !buf || buf_len <= 0) return -1;

    if (strcmp(key, "plugin_id") == 0) {
        return snprintf(buf, buf_len, "%s", inst->selected_plugin_id);
    }
    else if (strcmp(key, "plugin_name") == 0 || strcmp(key, "preset_name") == 0) {
        if (inst->current_plugin.plugin) {
            for (int i = 0; i < inst->plugin_list.count; i++) {
                if (strcmp(inst->plugin_list.items[i].id, inst->selected_plugin_id) == 0) {
                    return snprintf(buf, buf_len, "%s", inst->plugin_list.items[i].name);
                }
            }
        }
        return snprintf(buf, buf_len, "None");
    }
    else if (strcmp(key, "param_count") == 0) {
        return snprintf(buf, buf_len, "%d", clap_param_count(&inst->current_plugin));
    }
    else if (strncmp(key, "param_name_", 11) == 0) {
        int idx = atoi(key + 11);
        char name[64] = "";
        if (clap_param_info(&inst->current_plugin, idx, name, sizeof(name), NULL, NULL, NULL) == 0) {
            return snprintf(buf, buf_len, "%s", name);
        }
        return -1;
    }
    else if (strncmp(key, "param_value_", 12) == 0) {
        int idx = atoi(key + 12);
        double value = clap_param_get(&inst->current_plugin, idx);
        return snprintf(buf, buf_len, "%.3f", value);
    }
    /* ui_hierarchy for shadow parameter editor */
    else if (strcmp(key, "ui_hierarchy") == 0) {
        const char *hierarchy = "{"
            "\"modes\":null,"
            "\"levels\":{"
                "\"root\":{"
                    "\"list_param\":null,"
                    "\"count_param\":null,"
                    "\"name_param\":\"plugin_name\","
                    "\"children\":null,"
                    "\"knobs\":[],"
                    "\"params\":[]"
                "}"
            "}"
        "}";
        return snprintf(buf, buf_len, "%s", hierarchy);
    }

    return -1;
}

static audio_fx_api_v2_t g_fx_api_v2;

extern "C" audio_fx_api_v2_t* move_audio_fx_init_v2(const host_api_v1_t *host) {
    g_host = host;

    memset(&g_fx_api_v2, 0, sizeof(g_fx_api_v2));
    g_fx_api_v2.api_version = AUDIO_FX_API_VERSION_2;
    g_fx_api_v2.create_instance = v2_create_instance;
    g_fx_api_v2.destroy_instance = v2_destroy_instance;
    g_fx_api_v2.process_block = v2_process_block;
    g_fx_api_v2.set_param = v2_set_param;
    g_fx_api_v2.get_param = v2_get_param;

    v2_fx_log("CLAP FX V2 API initialized");

    return &g_fx_api_v2;
}
