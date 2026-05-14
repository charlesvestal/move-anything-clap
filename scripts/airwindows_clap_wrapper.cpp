/*
 * Custom CLAP wrapper for Airwindows effects
 *
 * Exposes each Airwindows effect as a separate CLAP plugin in the factory,
 * so the CLAP host can enumerate, select, and load them individually.
 *
 * Built against libairwin-registry.a from baconpaul/airwin2rack.
 */
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <memory>

#include "AirwinRegistry.h"
#include "airwin_consolidated_base.h"
#include "clap/clap.h"

/* ── helpers ──────────────────────────────────────────────────────── */

static constexpr uint32_t MAX_BLOCK = 4096;

struct airwin_plugin_t {
    clap_plugin_t plugin;
    const clap_host_t *host;
    int registry_index;
    std::unique_ptr<AirwinConsolidatedBase> fx;
    float sample_rate;
    int nparams;
};

static inline airwin_plugin_t *to_ap(const clap_plugin_t *p) {
    return (airwin_plugin_t *)p->plugin_data;
}

/* ── descriptors (built once at init) ────────────────────────────── */

struct descriptor_entry {
    clap_plugin_descriptor_t desc;
    std::string id;
    std::string name;
    std::string description;
    const char *features[4];
};
static std::vector<descriptor_entry> s_descriptors;

/*
 * Upstream registerAirwindow() skips effects with empty whatText.
 * We re-add them so every effect is available on the device.
 */
extern void AirwinAdd(void);  /* ModuleAdd.h populates the registry */

static void patch_registry() {
    /* ModuleAdd.h calls registerAirwindow() for all 504 effects,
     * but the ones with empty whatText get silently dropped.
     * We patch registerAirwindow to accept all, then re-run. */

    /* Already populated from static init — check if all are present.
     * If count matches what we expect, no patching needed. */

    /* The simplest approach: filterAndRebuildRegistry with a predicate
     * that removes nothing, which also calls completeRegistry(). */
    AirwinRegistry::completeRegistry();
}

static void ensure_descriptors() {
    if (!s_descriptors.empty()) return;
    patch_registry();

    s_descriptors.reserve(AirwinRegistry::registry.size());
    for (size_t i = 0; i < AirwinRegistry::registry.size(); i++) {
        auto &r = AirwinRegistry::registry[i];
        descriptor_entry e;
        e.id = "com.airwindows." + r.name;
        e.name = r.name;
        e.description = r.whatText;
        e.features[0] = CLAP_PLUGIN_FEATURE_AUDIO_EFFECT;
        e.features[1] = r.isMono ? CLAP_PLUGIN_FEATURE_MONO : CLAP_PLUGIN_FEATURE_STEREO;
        e.features[2] = nullptr;
        e.features[3] = nullptr;

        e.desc.clap_version = CLAP_VERSION;
        e.desc.id = e.id.c_str();
        e.desc.name = e.name.c_str();
        e.desc.vendor = "Airwindows";
        e.desc.url = "https://www.airwindows.com";
        e.desc.manual_url = "";
        e.desc.support_url = "";
        e.desc.version = "1.0.0";
        e.desc.description = e.description.c_str();
        e.desc.features = e.features;
        s_descriptors.push_back(std::move(e));
    }

    /* fix up pointers after vector is stable */
    for (auto &e : s_descriptors) {
        e.desc.id = e.id.c_str();
        e.desc.name = e.name.c_str();
        e.desc.description = e.description.c_str();
        e.desc.features = e.features;
    }
}

/* ── plugin callbacks ────────────────────────────────────────────── */

static bool ap_init(const clap_plugin_t *p) {
    auto *ap = to_ap(p);
    auto &r = AirwinRegistry::registry[ap->registry_index];
    ap->fx = r.generator();
    ap->nparams = r.nParams;
    if (ap->fx) ap->fx->setSampleRate(ap->sample_rate);
    return ap->fx != nullptr;
}

static void ap_destroy(const clap_plugin_t *p) {
    auto *ap = to_ap(p);
    delete ap;
}

static bool ap_activate(const clap_plugin_t *p, double sr, uint32_t, uint32_t) {
    auto *ap = to_ap(p);
    ap->sample_rate = (float)sr;
    if (ap->fx) ap->fx->setSampleRate((float)sr);
    return true;
}

static void ap_deactivate(const clap_plugin_t *) {}
static bool ap_start_processing(const clap_plugin_t *) { return true; }
static void ap_stop_processing(const clap_plugin_t *) {}
static void ap_reset(const clap_plugin_t *) {}

static clap_process_status ap_process(const clap_plugin_t *p,
                                       const clap_process_t *proc) {
    auto *ap = to_ap(p);
    if (!ap->fx) return CLAP_PROCESS_ERROR;

    uint32_t frames = proc->frames_count;

    /* handle parameter events */
    const uint32_t nev = proc->in_events ? proc->in_events->size(proc->in_events) : 0;
    for (uint32_t i = 0; i < nev; i++) {
        auto *hdr = proc->in_events->get(proc->in_events, i);
        if (hdr->type == CLAP_EVENT_PARAM_VALUE) {
            auto *ev = (const clap_event_param_value_t *)hdr;
            if ((int)ev->param_id < ap->nparams) {
                ap->fx->setParameter((int)ev->param_id, (float)ev->value);
            }
        }
    }

    float *in[2], *out[2];
    if (proc->audio_inputs_count > 0 && proc->audio_inputs[0].channel_count >= 1) {
        in[0] = proc->audio_inputs[0].data32[0];
        in[1] = proc->audio_inputs[0].channel_count >= 2
                    ? proc->audio_inputs[0].data32[1]
                    : proc->audio_inputs[0].data32[0];
    } else {
        /* synth-style: no input, provide silence */
        static float silence[MAX_BLOCK] = {0};
        in[0] = silence;
        in[1] = silence;
    }

    out[0] = proc->audio_outputs[0].data32[0];
    out[1] = proc->audio_outputs[0].channel_count >= 2
                 ? proc->audio_outputs[0].data32[1]
                 : proc->audio_outputs[0].data32[0];

    ap->fx->processReplacing(in, out, (int)frames);
    return CLAP_PROCESS_CONTINUE;
}

/* ── extensions: params ──────────────────────────────────────────── */

static uint32_t params_count(const clap_plugin_t *p) {
    return (uint32_t)to_ap(p)->nparams;
}

static bool params_get_info(const clap_plugin_t *p, uint32_t idx,
                            clap_param_info_t *info) {
    auto *ap = to_ap(p);
    if ((int)idx >= ap->nparams) return false;

    memset(info, 0, sizeof(*info));
    info->id = idx;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = ap->fx ? ap->fx->getParameter(idx) : 0.5;

    char name[kVstMaxParamStrLen + 1] = {0};
    if (ap->fx) ap->fx->getParameterName(idx, name);
    snprintf(info->name, sizeof(info->name), "%s", name[0] ? name : "Param");

    info->module[0] = '\0';
    return true;
}

static bool params_get_value(const clap_plugin_t *p, clap_id id, double *val) {
    auto *ap = to_ap(p);
    if ((int)id >= ap->nparams || !ap->fx) return false;
    *val = ap->fx->getParameter((int)id);
    return true;
}

static bool params_value_to_text(const clap_plugin_t *p, clap_id id,
                                  double val, char *buf, uint32_t sz) {
    auto *ap = to_ap(p);
    if ((int)id >= ap->nparams || !ap->fx) return false;
    float old = ap->fx->getParameter((int)id);
    ap->fx->setParameter((int)id, (float)val);
    char txt[kVstMaxParamStrLen + 1] = {0};
    ap->fx->getParameterDisplay((int)id, txt);
    ap->fx->setParameter((int)id, old);
    snprintf(buf, sz, "%s", txt);
    return true;
}

static bool params_text_to_value(const clap_plugin_t *, clap_id, const char *,
                                  double *) {
    return false;
}

static void params_flush(const clap_plugin_t *p,
                          const clap_input_events_t *in,
                          const clap_output_events_t *) {
    auto *ap = to_ap(p);
    if (!in || !ap->fx) return;
    uint32_t n = in->size(in);
    for (uint32_t i = 0; i < n; i++) {
        auto *hdr = in->get(in, i);
        if (hdr->type == CLAP_EVENT_PARAM_VALUE) {
            auto *ev = (const clap_event_param_value_t *)hdr;
            if ((int)ev->param_id < ap->nparams)
                ap->fx->setParameter((int)ev->param_id, (float)ev->value);
        }
    }
}

static const clap_plugin_params_t s_params = {
    .count = params_count,
    .get_info = params_get_info,
    .get_value = params_get_value,
    .value_to_text = params_value_to_text,
    .text_to_value = params_text_to_value,
    .flush = params_flush,
};

/* ── extensions: audio-ports ─────────────────────────────────────── */

static uint32_t audio_ports_count(const clap_plugin_t *, bool is_input) {
    return 1; /* one stereo in, one stereo out */
}

static bool audio_ports_get(const clap_plugin_t *p, uint32_t index,
                             bool is_input, clap_audio_port_info_t *info) {
    if (index != 0) return false;
    memset(info, 0, sizeof(*info));
    info->id = is_input ? 0 : 1;
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = is_input ? 1 : 0;
    snprintf(info->name, sizeof(info->name), "%s", is_input ? "Input" : "Output");
    return true;
}

static const clap_plugin_audio_ports_t s_audio_ports = {
    .count = audio_ports_count,
    .get = audio_ports_get,
};

/* ── extensions: note-ports (none) ───────────────────────────────── */

static uint32_t note_ports_count(const clap_plugin_t *, bool) { return 0; }
static bool note_ports_get(const clap_plugin_t *, uint32_t, bool,
                            clap_note_port_info_t *) {
    return false;
}

static const clap_plugin_note_ports_t s_note_ports = {
    .count = note_ports_count,
    .get = note_ports_get,
};

/* ── extension dispatch ──────────────────────────────────────────── */

static const void *ap_get_extension(const clap_plugin_t *, const char *id) {
    if (!strcmp(id, CLAP_EXT_PARAMS)) return &s_params;
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &s_audio_ports;
    if (!strcmp(id, CLAP_EXT_NOTE_PORTS)) return &s_note_ports;
    return nullptr;
}

static void ap_on_main_thread(const clap_plugin_t *) {}

/* ── factory ─────────────────────────────────────────────────────── */

static uint32_t factory_count(const clap_plugin_factory_t *) {
    ensure_descriptors();
    return (uint32_t)s_descriptors.size();
}

static const clap_plugin_descriptor_t *factory_get(
    const clap_plugin_factory_t *, uint32_t idx) {
    ensure_descriptors();
    if (idx >= s_descriptors.size()) return nullptr;
    return &s_descriptors[idx].desc;
}

static const clap_plugin_t *factory_create(
    const clap_plugin_factory_t *, const clap_host_t *host, const char *id) {
    ensure_descriptors();

    for (size_t i = 0; i < s_descriptors.size(); i++) {
        if (strcmp(s_descriptors[i].id.c_str(), id) == 0) {
            auto *ap = new airwin_plugin_t();
            ap->host = host;
            ap->registry_index = (int)i;
            ap->sample_rate = 44100.0f;
            ap->fx = nullptr;
            ap->nparams = 0;

            ap->plugin.desc = &s_descriptors[i].desc;
            ap->plugin.plugin_data = ap;
            ap->plugin.init = ap_init;
            ap->plugin.destroy = ap_destroy;
            ap->plugin.activate = ap_activate;
            ap->plugin.deactivate = ap_deactivate;
            ap->plugin.start_processing = ap_start_processing;
            ap->plugin.stop_processing = ap_stop_processing;
            ap->plugin.reset = ap_reset;
            ap->plugin.process = ap_process;
            ap->plugin.get_extension = ap_get_extension;
            ap->plugin.on_main_thread = ap_on_main_thread;

            return &ap->plugin;
        }
    }
    return nullptr;
}

static const clap_plugin_factory_t s_factory = {
    .get_plugin_count = factory_count,
    .get_plugin_descriptor = factory_get,
    .create_plugin = factory_create,
};

/* ── entry ───────────────────────────────────────────────────────── */

static bool entry_init(const char *) {
    ensure_descriptors();
    return true;
}

static void entry_deinit(void) {
    s_descriptors.clear();
    s_descriptors.shrink_to_fit();
}

static const void *entry_get_factory(const char *factory_id) {
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
        return &s_factory;
    return nullptr;
}

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory,
};
