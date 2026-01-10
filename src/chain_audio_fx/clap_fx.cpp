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

/* === Audio FX Entry Point === */

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
