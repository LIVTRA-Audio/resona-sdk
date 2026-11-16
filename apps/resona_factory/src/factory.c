/*
 * Resona Box factory app: the box's own UI and operation logic, as a box app on the platform
 * API (resona/platform.h, version 1). It is the template for your own: copy the project and
 * change what you like. GPL-3.0 licensed.
 *
 * A preset is one of two things: the factory chain (the built-ins Gate, Compressor, Resona amp,
 * NAM, Cabinet, each once, in any order) or one installed RPK effect on its own.
 *
 * Pages: main (preset, volume, settings, one tile per pedal of the factory chain and an
 * EFFECT tile), effect editor, settings, Global EQ, the EFFECT picker. A one-RPK preset shows
 * that effect's controls on the main page instead of pedals.
 * Encoder 1: turn moves, click confirms/edits, double click saves, long press goes home (or
 * moves a pedal on the main page). Editing (the playing preset only; saving is the usual save):
 *   - choose the effect: the EFFECT tile (after the pedals, or alone on an empty preset) or the
 *     CHANGE EFFECT tile of a one-RPK preset opens a list: FACTORY CHAIN, then the installed RPK
 *     effects, the current one marked. Click plays the chosen one at its defaults, long press
 *     goes back.
 *   - move: long press on a pedal of the factory chain, turn to move it, click when done.
 * Edits are jobs (program_edit): UPDATING CHAIN shows while one runs, a failure is a toast.
 *   - a one-RPK effect that draws its own screen: click on its EFFECT tile opens that screen
 *     full frame (both encoders go to the effect, except Encoder 1 long press: back to the
 *     tiles, and double press: save); long press on the tile switches the effect on/off.
 * Encoder 2: turn changes the mapped
 * parameter, press and long press run the actions chosen in Settings (preset up, an effect
 * on/off, bypass ...); long press on an editable parameter maps Encoder 2 to it.
 *
 * Everything the box plays (chain, parameters, models, presets, Global EQ, volume) is the
 * platform's; this file keeps mirrors of it for drawing, refreshed from the platform.
 */
#include <resona/libc.h>
#include <resona/math.h>
#include <resona/platform.h>

#include "renderer.h"

#define MAX_EFFECTS RESONA_CHAIN_MAX
#define ENGINES 5U      /* built-in engines RESONA_ENGINE_GATE..RESONA_ENGINE_CAB */
#define MODEL_BANKS 3U  /* Resona amp profiles, NAM, Cabinet (IR) */
#define MODEL_SLOTS 30U
#define NONE 0xFFU
#define BLINK_MS 350U
#define SAVED_MS 1500U
#define POPUP_MS 1500U
#define ERROR_MS 3500U
/* Browse first: names scroll instantly; the sound loads once the knob rests. */
#define PRESET_APPLY_MS 350U
#define LED_PULSE_MS 350U
#define LED_BYPASS_FLASH_MS 200U
#define TIMER_PRESET 1U

enum { MAIN_PRESET, MAIN_VOLUME, MAIN_GLOBAL, MAIN_EFFECT0 };
enum { GESTURE_PRESS, GESTURE_LONG, GESTURE_COUNT };
enum {
    ACTION_PRESET_UP, ACTION_PRESET_DOWN, ACTION_GATE, ACTION_COMP, ACTION_RESONA, ACTION_NAM,
    ACTION_CAB, ACTION_BYPASS, ACTION_NONE, ACTION_GLOBAL_EQ, ACTION_COUNT
};
enum {
    SETTING_GLOBAL_EQ, SETTING_LOOPBACK, SETTING_BLUETOOTH, SETTING_BT_VOLUME, SETTING_USB_VOLUME,
    SETTING_INPUT_GAIN, SETTING_MIDI_CHANNEL, SETTING_ENC2_PRESS, SETTING_ENC2_LONG, SETTING_FIRMWARE, SETTING_COUNT
};
enum { MAP_EFFECT = 0, MAP_GLOBAL_EQ = 1 };

/* Indexed by engine - RESONA_ENGINE_GATE. */
static const char *const g_engine_titles[ENGINES] = {"GATE", "COMP", "RESONA", "NAM", "CAB"};
static const uint8_t g_engine_rows[ENGINES] = {2U, 6U, 7U, 4U, 3U};
static const char *const g_setting_names[SETTING_COUNT] = {
    "GLOBAL EQ", "LOOPBACK", "BT", "BT VOLUME", "USB VOLUME",
    "INPUT GAIN", "MIDI CHANNEL", "ENC2 PRESS", "ENC2 LONG", "FIRMWARE",
};
static const char *const g_action_names[ACTION_COUNT] = {
    "PRESET +", "PRESET -", "GATE", "COMP", "RESONA", "NAM", "CAB", "BYPASS", "NONE", "GLOBAL EQ",
};
static const uint8_t g_default_actions[GESTURE_COUNT] = {ACTION_PRESET_UP, ACTION_BYPASS};

_Static_assert(SETTING_COUNT <= BOX_UI_MAX_ROWS, "settings rows must fit the renderer buffers");
_Static_assert(MAX_EFFECTS < BOX_UI_MAX_EFFECTS, "the chain and its EFFECT tile must fit the renderer");

/* What the app keeps in its own storage. Effect mappings of Encoder 2 belong to the
 * platform (program_map_encoder); the app only stores a Global EQ mapping. */
#define STORE_MAGIC 0x31414652U /* 'RFA1' */
typedef struct {
    uint32_t magic;
    uint8_t actions[GESTURE_COUNT];
    uint8_t map_valid, map_kind, map_reserved, map_row;
} store_t;

static const resona_platform *P;
static uint16_t *g_fb;
static uint32_t g_stride;
static resona_device_info g_dev;
static resona_chain_view g_chain;
static resona_program_info g_program = {
    .active_preset = -1,
    .encoder_effect = RESONA_PROGRAM_NO_PARAMETER,
    .encoder_parameter = RESONA_PROGRAM_NO_PARAMETER,
};
static resona_program_job g_job;
static uint32_t g_requested_job, g_completed_job;
static uint32_t g_edit_job; /* the chain edit in flight, 0 if none */
static int g_edit_to = -1;  /* the effect to select when it completes */
static bool g_edit_keep_reorder; /* a move keeps reorder mode on */

static bool program_loading(void)
{
    resona_program_job job;

    memset(&job, 0, sizeof(job));
    job.version = 1U;
    job.size = sizeof(job);
    if (P->program_job(&job)) {
        return false;
    }
    g_job = job;
    return (job.state >= RESONA_PROGRAM_QUEUED && job.state <= RESONA_PROGRAM_STARTING) || job.queued_id != 0;
}

static uint32_t g_chain_poll_ms;

/* ---- mirrors of the platform's state --------------------------------------- */

static int g_volume;
static int g_bypass;
static uint32_t g_eq_enabled;
static int8_t g_eq_bands[BOX_UI_GLOBAL_EQ_BANDS];
static int8_t g_eq_level;
static int g_preset_slot = -1; /* active preset, -1 none */
static char g_live_name[RESONA_PRESET_NAME + 1];

/* models per bank (Resona, NAM, Cab): entry ids and names, from the platform's entry list */
static uint32_t g_model_ids[MODEL_BANKS][MODEL_SLOTS];
static char g_model_names[MODEL_BANKS][MODEL_SLOTS][24];
static uint32_t g_model_count[MODEL_BANKS];

/* generic (RPK) parameters of the effect being listed */
static resona_effect_param g_pack_pi[BOX_UI_MAX_ROWS];
static float g_pack_val[BOX_UI_MAX_ROWS];

/* ---- view state (as in the firmware UI) --------------------------------------- */

static box_view_t g_view;
static uint8_t g_selected_main_item;
static uint8_t g_selected_effect;
static uint8_t g_tile_first; /* first effect tile shown when the chain has more than fit */
static uint8_t g_selected_row;
static box_editor_focus_t g_editor_focus;
static uint8_t g_editor_page;
static uint8_t g_settings_selected;
static uint8_t g_global_eq_selected;
static uint8_t g_gesture_actions[GESTURE_COUNT];
static int g_model_edit; /* index into g_model_ids[bank of selected effect], or -1 */
static bool g_editing;
static bool g_reorder_mode;
static bool g_preset_browse;
static bool g_preset_save;
static bool g_preset_conflict;
static bool g_preset_blink_visible = true;
static uint32_t g_preset_blink_next;
static const char *g_save_error;
static uint32_t g_save_error_until;
static bool g_preset_saved;
static uint32_t g_preset_saved_until;
static int g_preset_apply_pending = -1;
static int g_preset_preview_slot = -1;
static bool g_preset_preview_valid;
static int g_preset_target;
static bool g_enc2_map_valid; /* Global EQ mapping (app-owned) */
static uint8_t g_enc2_map_kind;
static uint8_t g_enc2_map_row;
static char g_enc2_popup_title[24];
static char g_enc2_popup_value[16];
static uint32_t g_enc2_popup_until;
static uint32_t g_volume_popup_until;
static uint32_t g_led_until;
static uint32_t g_led_pulse;
static box_ui_render_state_t g_main_state;
static bool g_meter_valid;
static uint8_t g_meter_in, g_meter_out;
static bool g_dirty = true;
static bool g_pack_screen; /* the selected RPK effect drew its own screen last time */
static int g_pack_screen_effect = -1;
/* One-RPK preset: does its effect draw its own screen (-1 not asked yet for this load), and is
 * that screen open (full frame, from the EFFECT tile)? */
static int8_t g_own_screen = -1;
static char g_own_screen_name[24];
static bool g_screen_open;

/* Choosing the preset's effect: the EFFECT picker (a view of its own) and a toast. */
#define PICKER_MAX 32U
#define TOAST_MS 2500U
typedef struct {
    uint32_t source, entry; /* RESONA_CHAIN_BUILTIN: the factory chain */
    char name[17];
} picker_item_t;
static picker_item_t g_picker[PICKER_MAX];
static uint8_t g_picker_count;
static uint8_t g_picker_selected;
static int g_picker_current = -1; /* the item the preset plays now, -1 if none */
static const char *g_toast;
static uint32_t g_toast_until;

static uint32_t now(void)
{
    return P->millis();
}

static int clamp_int(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint8_t step_index_clamped(uint8_t current, int delta, uint8_t count)
{
    return count == 0U ? 0U : (uint8_t)clamp_int((int)current + delta, 0, (int)count - 1);
}

static void show_toast(const char *message)
{
    g_toast = message;
    g_toast_until = now() + TOAST_MS;
    g_dirty = true;
}

/* The toast for an edit refused at submit or failed as a job. */
static const char *chain_edit_error(int result, bool move)
{
    return result == RESONA_ERR_BUDGET ? "NOT ENOUGH CPU" :
           result == RESONA_ERR_NO_MEMORY ? "NOT ENOUGH MEMORY" :
           move ? "MOVE FAILED" : "EFFECT CHANGE FAILED";
}

/* ---- the chain ------------------------------------------------------------------ */

static uint8_t effect_count(void)
{
    return (uint8_t)(g_chain.count < MAX_EFFECTS ? g_chain.count : MAX_EFFECTS);
}

static const resona_chain_effect *effect_at(uint8_t e)
{
    return e < effect_count() ? &g_chain.effects[e] : NULL;
}

/* A built-in engine with its own artwork and value formatting. */
static bool effect_builtin(uint8_t e)
{
    const resona_chain_effect *x = effect_at(e);

    return x && x->source == RESONA_CHAIN_BUILTIN &&
           x->engine >= RESONA_ENGINE_GATE && x->engine <= RESONA_ENGINE_CAB;
}

static uint32_t effect_engine(uint8_t e)
{
    return effect_builtin(e) ? g_chain.effects[e].engine : RESONA_ENGINE_NONE;
}

static bool effect_is_rpk(uint8_t e)
{
    const resona_chain_effect *x = effect_at(e);

    return x && x->source == RESONA_CHAIN_RPK;
}

/* A chain of exactly one RPK effect shows that effect's controls on the main page. */
static bool chain_single_rpk(void)
{
    return effect_count() == 1U && effect_is_rpk(0U);
}

/* The main page has an EFFECT tile (opens the picker) at item MAIN_EFFECT0 + count, after the
 * pedals of the factory chain or alone on an empty preset. A one-RPK preset has it among its
 * controls (CHANGE EFFECT) instead. */
static bool effect_slot_shown(void)
{
    return !chain_single_rpk();
}

static uint8_t main_tile_count(void)
{
    return (uint8_t)(effect_count() + (effect_slot_shown() ? 1U : 0U));
}

static bool effect_slot_selected(void)
{
    return effect_slot_shown() && g_selected_main_item == MAIN_EFFECT0 + effect_count();
}

static bool effect_enabled(uint8_t e)
{
    const resona_chain_effect *x = effect_at(e);

    return x && x->enabled;
}

static const char *effect_title(uint8_t e)
{
    const resona_chain_effect *x = effect_at(e);

    if (!x) {
        return "EFFECT";
    }
    if (effect_builtin(e)) {
        return g_engine_titles[x->engine - RESONA_ENGINE_GATE];
    }
    return x->name[0] ? x->name : "RPK";
}

/* The model bank of the effect (0 Resona, 1 NAM, 2 Cab), or -1 if it has no models. */
static int model_bank(uint8_t e)
{
    uint32_t engine = effect_engine(e);

    return engine >= RESONA_ENGINE_AMP && engine <= RESONA_ENGINE_CAB ? (int)(engine - RESONA_ENGINE_AMP) : -1;
}

static bool effect_has_model(uint8_t e)
{
    return model_bank(e) >= 0;
}

static uint8_t effect_variant_count(uint8_t e)
{
    return effect_engine(e) == RESONA_ENGINE_GATE ? 2U : 1U;
}

static uint8_t effect_row_count(uint8_t e)
{
    const resona_chain_effect *x = effect_at(e);
    uint32_t n;

    if (!x) {
        return 0U;
    }
    n = x->parameter_count;
    if (effect_builtin(e) && n > g_engine_rows[x->engine - RESONA_ENGINE_GATE]) {
        n = g_engine_rows[x->engine - RESONA_ENGINE_GATE];
    }
    return (uint8_t)(n < BOX_UI_MAX_ROWS ? n : BOX_UI_MAX_ROWS);
}

/* ---- sync from the platform ---------------------------------------------------- */

static void scan_models(void)
{
    int n = P->entry_count();

    memset(g_model_count, 0, sizeof(g_model_count));
    for (int i = 0; i < n; ++i) {
        resona_entry_info en;
        uint32_t bank;

        if (P->entry_info((uint32_t)i, &en) != RESONA_OK) {
            continue;
        }
        bank = en.kind == RESONA_ENTRY_AMP ? 0U : en.kind == RESONA_ENTRY_NAM ? 1U :
               en.kind == RESONA_ENTRY_IR ? 2U : MODEL_BANKS;
        if (bank < MODEL_BANKS && g_model_count[bank] < MODEL_SLOTS) {
            g_model_ids[bank][g_model_count[bank]] = en.id;
            strncpy(g_model_names[bank][g_model_count[bank]], en.name, 23);
            g_model_count[bank]++;
        }
    }
}

static bool read_chain(resona_chain_view *out)
{
    memset(out, 0, sizeof(*out));
    out->version = 1U;
    out->size = sizeof(*out);
    if (P->program_chain(out) != RESONA_OK) {
        return false;
    }
    if (out->count > MAX_EFFECTS) {
        out->count = MAX_EFFECTS;
    }
    return true;
}

static void sync(void)
{
    resona_program_info programme;
    resona_chain_view chain;
    int32_t iv;
    uint8_t count;

    if (program_loading()) {
        return;
    }
    memset(&programme, 0, sizeof(programme));
    programme.version = 1U;
    programme.size = sizeof(programme);
    if (!P->program_info(&programme)) {
        g_program = programme;
    }
    if (read_chain(&chain)) {
        g_chain = chain;
    }
    count = effect_count();
    if (g_selected_main_item >= MAIN_EFFECT0 + main_tile_count()) {
        g_selected_main_item = main_tile_count() ? (uint8_t)(MAIN_EFFECT0 + main_tile_count() - 1U) : MAIN_PRESET;
        g_reorder_mode = false;
    }
    if (g_reorder_mode && g_selected_main_item >= MAIN_EFFECT0 + count) {
        g_reorder_mode = false;
    }
    if (g_selected_effect >= count) {
        g_selected_effect = count ? (uint8_t)(count - 1U) : 0U;
    }
    if (g_view == BOX_VIEW_EDITOR && count == 0U) {
        g_view = BOX_VIEW_MAIN;
        g_editing = false;
    }
    if (P->setting_get(RESONA_SETTING_VOLUME, &iv) == RESONA_OK) {
        g_volume = iv;
    }
    if (P->setting_get(RESONA_SETTING_BYPASS, &iv) == RESONA_OK) {
        g_bypass = iv;
    }
    P->global_eq_get(&g_eq_enabled, g_eq_bands, &g_eq_level);
    g_preset_slot = g_program.active_preset;
    memcpy(g_live_name, g_program.name, sizeof(g_live_name));
    g_live_name[RESONA_PRESET_NAME] = '\0';
}

/* The model list position of the effect's selected model, or -1. */
static int model_selected(uint8_t e)
{
    int bank = model_bank(e);
    const resona_chain_effect *x = effect_at(e);

    if (bank < 0 || !x || x->model < 0) {
        return -1;
    }
    for (uint32_t i = 0; i < g_model_count[bank]; ++i) {
        if (RESONA_ENTRY_INDEX(g_model_ids[bank][i]) == (uint32_t)x->model) {
            return (int)i;
        }
    }
    return -1;
}

static int model_browse_origin(uint8_t e)
{
    int sel = model_selected(e);
    int bank = model_bank(e);

    return sel >= 0 ? sel : (bank >= 0 && g_model_count[bank] ? 0 : -1);
}

/* ---- effect parameters (through the platform) ------------------------------------ */

/* A parameter as 0..1 of its range. */
static bool param_norm_get(uint8_t e, uint8_t i, float *norm)
{
    resona_effect_param q;
    float v, span, n;

    if (P->program_effect_param_info(e, i, &q) != RESONA_OK ||
        P->program_effect_param_get(e, i, &v) != RESONA_OK) {
        return false;
    }
    span = q.max - q.min;
    n = span > 0.0f ? (v - q.min) / span : 0.0f;
    *norm = n < 0.0f ? 0.0f : (n > 1.0f ? 1.0f : n);
    return true;
}

static int param_norm_set(uint8_t e, uint8_t i, float norm)
{
    resona_effect_param q;

    if (P->program_effect_param_info(e, i, &q) != RESONA_OK) {
        return -1;
    }
    norm = norm < 0.0f ? 0.0f : (norm > 1.0f ? 1.0f : norm);
    return P->program_effect_param_set(e, i, q.min + norm * (q.max - q.min)) == RESONA_OK ? 0 : -1;
}

/* A built-in knob as 0..100. */
static uint8_t param_percent(uint8_t e, uint8_t i)
{
    float n = 0.0f;

    (void)param_norm_get(e, i, &n);
    return (uint8_t)clamp_int((int)(n * 100.0f + 0.5f), 0, 100);
}

/* The Cabinet level (0..1 over -12..+12 dB) in tenths of a dB. */
static int cab_level_tenths(uint8_t e)
{
    float n = 0.5f;

    (void)param_norm_get(e, 2U, &n);
    return (int)(n * 240.0f + 0.5f) - 120;
}

/* One detent on a generic parameter: its step, a toggle or choice flip, or 1 % of its range. */
static void nudge_effect_param(uint8_t e, uint32_t index, int delta)
{
    resona_effect_param q;
    float v;
    float step;

    if (delta == 0 || P->program_effect_param_info(e, index, &q) != RESONA_OK ||
        P->program_effect_param_get(e, index, &v) != RESONA_OK) {
        return;
    }
    if (q.flags & RESONA_PARAM_TOGGLE) {
        step = q.max - q.min;
    } else if (q.flags & RESONA_PARAM_CHOICE) {
        step = 1.0f;
    } else if ((q.flags & RESONA_PARAM_STEPPED) && q.step > 0.0f) {
        step = q.step;
    } else {
        step = (q.max - q.min) / 100.0f;
    }
    v += step * (float)delta;
    v = v < q.min ? q.min : (v > q.max ? q.max : v);
    (void)P->program_effect_param_set(e, index, v);
    g_dirty = true;
}

static int set_volume(int v)
{
    return P->setting_set(RESONA_SETTING_VOLUME, clamp_int(v, 0, 100)) == RESONA_OK ? 0 : -1;
}

static void toggle_effect(uint8_t e)
{
    if (e < effect_count()) {
        (void)P->program_effect_enable(e, effect_enabled(e) ? 0U : 1U);
    }
}

static int eq_apply(bool persist)
{
    return P->global_eq_set(g_eq_enabled, g_eq_bands, g_eq_level, persist ? 1U : 0U) == RESONA_OK ? 0 : -1;
}

/* Encoder 2 mappings: a Global EQ control (the app's) wins over the programme's parameter. */
static bool eq_mapped(void)
{
    return g_enc2_map_valid && g_enc2_map_kind == MAP_GLOBAL_EQ;
}

static bool effect_mapped(void)
{
    return !eq_mapped() && g_program.encoder_parameter != RESONA_PROGRAM_NO_PARAMETER &&
           g_program.encoder_effect < effect_count();
}

static bool effect_param_mapped(uint8_t e, uint32_t index)
{
    return effect_mapped() && g_program.encoder_effect == e && g_program.encoder_parameter == index;
}

/* ---- formatting (as in the firmware UI) ---------------------------------------- */

static void set_row(box_ui_row_t rows[BOX_UI_MAX_ROWS], uint8_t row, const char *label,
                    const char *value, bool mutable_)
{
    rows[row].label = label;
    rows[row].value = value;
    rows[row].counter = NULL;
    rows[row].mutable = mutable_;
    rows[row].mapped = false;
}

static void format_percent(char out[16], uint8_t value)
{
    snprintf(out, 16U, "%u", (unsigned)value);
}

static void format_db_tenths(char out[16], int tenths)
{
    unsigned magnitude = (unsigned)(tenths < 0 ? -tenths : tenths);

    snprintf(out, 16U, "%s%u.%u DB", tenths < 0 ? "-" : "", magnitude / 10U, magnitude % 10U);
}

static void format_knob_tenths(char out[16], uint8_t value)
{
    snprintf(out, 16U, "%u.%u", (unsigned)value / 10U, (unsigned)value % 10U);
}

static void format_pm10_db(char out[16], uint8_t raw)
{
    format_db_tenths(out, ((int)raw - 50) * 2);
}

static void format_ratio(char out[16], uint8_t raw)
{
    int tenths = 10 + (int)raw * 190 / 100;

    snprintf(out, 16U, "%d.%d:1", tenths / 10, tenths % 10);
}

static void format_eq_steps(char out[16], int steps)
{
    unsigned magnitude = (unsigned)(steps < 0 ? -steps : steps);

    snprintf(out, 16U, "%s%u.%u", steps < 0 ? "-" : "+", magnitude / 2U, (magnitude & 1U) ? 5U : 0U);
}

static void format_ir_cut_frequency(char out[16], uint8_t param, uint8_t raw)
{
    float minimum = param == 0U ? 20.0f : 2000.0f;
    float maximum = param == 0U ? 500.0f : 20000.0f;
    float frequency = resona_expf(resona_logf(minimum) + ((float)raw / 100.0f) *
                                  (resona_logf(maximum) - resona_logf(minimum)));
    unsigned hz = (unsigned)(frequency + 0.5f);

    if (param == 0U && frequency < 100.0f) {
        unsigned tenths_hz = (unsigned)(frequency * 10.0f + 0.5f);

        if ((tenths_hz % 10U) == 0U) {
            snprintf(out, 16U, "%u HZ", tenths_hz / 10U);
        } else {
            snprintf(out, 16U, "%u.%u HZ", tenths_hz / 10U, tenths_hz % 10U);
        }
    } else if (hz >= 10000U) {
        unsigned tenths_khz = (hz + 50U) / 100U;

        if ((tenths_khz % 10U) == 0U) {
            snprintf(out, 16U, "%u KHZ", tenths_khz / 10U);
        } else {
            snprintf(out, 16U, "%u.%u KHZ", tenths_khz / 10U, tenths_khz % 10U);
        }
    } else {
        snprintf(out, 16U, "%u HZ", hz);
    }
}

/* A generic parameter as shown: one decimal and its unit, ON/OFF, or a choice number. */
static void format_pack_value(char out[16], const resona_effect_param *pi, float v)
{
    int tenths;
    const char *sign;

    if (pi->flags & RESONA_PARAM_TOGGLE) {
        snprintf(out, 16U, "%s", v > pi->min ? "ON" : "OFF");
        return;
    }
    if (pi->flags & RESONA_PARAM_CHOICE) {
        snprintf(out, 16U, "%d", (int)v);
        return;
    }
    tenths = (int)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));
    sign = tenths < 0 ? "-" : "";
    if (tenths < 0) {
        tenths = -tenths;
    }
    if (tenths % 10 == 0 || tenths >= 10000) {
        snprintf(out, 16U, "%s%d %s", sign, tenths / 10, pi->unit);
    } else {
        snprintf(out, 16U, "%s%d.%d %s", sign, tenths / 10, tenths % 10, pi->unit);
    }
    for (char *c = out; *c; ++c) {
        if (*c >= 'a' && *c <= 'z') {
            *c = (char)(*c - 'a' + 'A');
        }
    }
}

/* The parameter rows of effect e: built-ins with their own units, RPK effects generic. */
static uint8_t build_editor_rows(uint8_t e, box_ui_row_t rows[BOX_UI_MAX_ROWS],
                                 char values[BOX_UI_MAX_ROWS][16])
{
    uint8_t n = effect_row_count(e);
    uint32_t engine = effect_engine(e);
    uint8_t p[6];

    if (!effect_builtin(e)) {
        uint8_t shown = 0U;

        for (uint8_t i = 0U; i < n; ++i, ++shown) {
            memset(&g_pack_pi[i], 0, sizeof(g_pack_pi[i]));
            g_pack_val[i] = 0.0f;
            if (P->program_effect_param_info(e, i, &g_pack_pi[i]) != RESONA_OK) {
                break;
            }
            g_pack_pi[i].name[sizeof(g_pack_pi[i].name) - 1U] = '\0';
            g_pack_pi[i].unit[sizeof(g_pack_pi[i].unit) - 1U] = '\0';
            (void)P->program_effect_param_get(e, i, &g_pack_val[i]);
            format_pack_value(values[i], &g_pack_pi[i], g_pack_val[i]);
            set_row(rows, i, g_pack_pi[i].name, values[i], true);
        }
        return shown;
    }
    if (engine == RESONA_ENGINE_AMP) {
        for (uint8_t i = 0U; i < n; ++i) {
            resona_effect_param *info = &g_pack_pi[i];
            float norm;
            if (P->program_effect_param_info(e, i, info) != RESONA_OK ||
                P->program_effect_param_get(e, i, &norm) != RESONA_OK) {
                return i;
            }
            info->name[sizeof(info->name) - 1U] = '\0';
            info->unit[sizeof(info->unit) - 1U] = '\0';
            uint8_t value = (uint8_t)clamp_int((int)(norm * 100.0f + 0.5f), 0, 100);
            if (strcmp(info->unit, "dB") == 0) {
                format_pm10_db(values[i], value);
            } else if (strcmp(info->name, "LEVEL") == 0) {
                format_percent(values[i], value);
            } else {
                format_knob_tenths(values[i], value);
            }
            set_row(rows, i, info->name, values[i], true);
        }
        return n;
    }
    for (uint8_t i = 0U; i < n && i < 6U; ++i) {
        p[i] = param_percent(e, i);
    }
    switch (engine) {
    case RESONA_ENGINE_GATE:
        if (n < 2U) return 0U;
        if (g_chain.effects[e].variant == 1U) {
            format_db_tenths(values[0], -60 + (int)p[0] * 120 / 100);
            snprintf(values[1], 16U, "%u MS", (unsigned)(20U + (uint32_t)p[1] * 280U / 100U));
            set_row(rows, 0U, "SENSITIVITY", values[0], true);
        } else {
            snprintf(values[0], 16U, "%d DB", -100 + (int)p[0]);
            snprintf(values[1], 16U, "%u MS", (unsigned)(5U + (uint32_t)p[1] * 95U / 100U));
            set_row(rows, 0U, "THRESHOLD", values[0], true);
        }
        set_row(rows, 1U, "RELEASE", values[1], true);
        return 2U;
    case RESONA_ENGINE_COMP:
        if (n < 6U) return 0U;
        format_db_tenths(values[0], -600 + (int)p[0] * 6);
        format_ratio(values[1], p[1]);
        format_db_tenths(values[2], (int)p[2] * 240 / 100);
        snprintf(values[3], 16U, "%u MS", (unsigned)p[3] * 200U / 100U);
        snprintf(values[4], 16U, "%u MS", (unsigned)p[4] * 1000U / 100U);
        format_db_tenths(values[5], -240 + (int)p[5] * 48 / 10);
        set_row(rows, 0U, "THRESHOLD", values[0], true);
        set_row(rows, 1U, "RATIO", values[1], true);
        set_row(rows, 2U, "KNEE", values[2], true);
        set_row(rows, 3U, "ATTACK", values[3], true);
        set_row(rows, 4U, "RELEASE", values[4], true);
        set_row(rows, 5U, "MAKEUP", values[5], true);
        return 6U;
    case RESONA_ENGINE_NAM:
        if (n < 4U) return 0U;
        format_db_tenths(values[0], ((int)p[0] - 50) * 2);
        format_knob_tenths(values[1], p[1]);
        format_knob_tenths(values[2], p[2]);
        format_knob_tenths(values[3], p[3]);
        set_row(rows, 0U, "LEVEL", values[0], true);
        set_row(rows, 1U, "BASS", values[1], true);
        set_row(rows, 2U, "MID", values[2], true);
        set_row(rows, 3U, "TREBLE", values[3], true);
        return 4U;
    case RESONA_ENGINE_CAB:
        if (n < 3U) return 0U;
        format_ir_cut_frequency(values[0], 0U, p[0]);
        format_ir_cut_frequency(values[1], 1U, p[1]);
        format_db_tenths(values[2], cab_level_tenths(e));
        set_row(rows, 0U, "LOW CUT", values[0], true);
        set_row(rows, 1U, "HIGH CUT", values[1], true);
        set_row(rows, 2U, "LEVEL", values[2], true);
        return 3U;
    default:
        return 0U;
    }
}

static void format_setting_values(char values[BOX_UI_MAX_ROWS][16])
{
    int32_t v = 0;

    P->setting_get(RESONA_SETTING_LOOPBACK, &v);
    snprintf(values[SETTING_LOOPBACK], 16, "%s", v ? "ON" : "OFF");
    P->setting_get(RESONA_SETTING_INPUT_GAIN, &v);
    snprintf(values[SETTING_INPUT_GAIN], 16, "%+d DB", (int)v);
    snprintf(values[SETTING_GLOBAL_EQ], 16, "OPEN");
    P->setting_get(RESONA_SETTING_USB_VOLUME, &v);
    snprintf(values[SETTING_USB_VOLUME], 16, "%d", (int)v);
    P->setting_get(RESONA_SETTING_BT_ENABLE, &v);
    snprintf(values[SETTING_BLUETOOTH], 16, "%s", v ? "ON" : "OFF");
    P->setting_get(RESONA_SETTING_BT_VOLUME, &v);
    snprintf(values[SETTING_BT_VOLUME], 16, "%d", (int)v);
    P->setting_get(RESONA_SETTING_MIDI_CHANNEL, &v);
    if (v == 0) {
        snprintf(values[SETTING_MIDI_CHANNEL], 16, "OMNI");
    } else {
        snprintf(values[SETTING_MIDI_CHANNEL], 16, "%d", (int)v);
    }
    snprintf(values[SETTING_ENC2_PRESS], 16, "%s", g_action_names[g_gesture_actions[GESTURE_PRESS]]);
    snprintf(values[SETTING_ENC2_LONG], 16, "%s", g_action_names[g_gesture_actions[GESTURE_LONG]]);
    snprintf(values[SETTING_FIRMWARE], 16, "%s", g_dev.platform_version);
}

static uint8_t build_settings_rows(box_ui_row_t rows[BOX_UI_MAX_ROWS], char values[BOX_UI_MAX_ROWS][16])
{
    format_setting_values(values);
    for (uint8_t i = 0U; i < SETTING_COUNT; ++i) {
        set_row(rows, i, g_setting_names[i], values[i], i != SETTING_FIRMWARE);
    }
    return SETTING_COUNT;
}

/* ---- storage of the app's own settings ------------------------------------------- */

static void store_save(void)
{
    store_t st;

    memset(&st, 0, sizeof(st));
    st.magic = STORE_MAGIC;
    st.actions[0] = g_gesture_actions[0];
    st.actions[1] = g_gesture_actions[1];
    st.map_valid = g_enc2_map_valid;
    st.map_kind = g_enc2_map_kind;
    st.map_row = g_enc2_map_row;
    P->app_store_write(0, &st, sizeof(st));
}

static void store_load(void)
{
    store_t st;

    g_gesture_actions[0] = g_default_actions[0];
    g_gesture_actions[1] = g_default_actions[1];
    g_enc2_map_valid = false;
    g_enc2_map_kind = MAP_EFFECT;
    g_enc2_map_row = 0U;
    if (P->app_store_read(0, &st, sizeof(st)) != RESONA_OK || st.magic != STORE_MAGIC) {
        return;
    }
    for (int i = 0; i < GESTURE_COUNT; ++i) {
        if (st.actions[i] < ACTION_COUNT) {
            g_gesture_actions[i] = st.actions[i];
        }
    }
    /* Effect mappings stored by older versions are the platform's now: ignored. */
    if (st.map_kind == MAP_GLOBAL_EQ && st.map_row >= 1U && st.map_row <= BOX_UI_GLOBAL_EQ_BANDS + 1U) {
        g_enc2_map_valid = st.map_valid != 0;
        g_enc2_map_kind = st.map_kind;
        g_enc2_map_row = st.map_row;
    }
}

/* ---- rendering --------------------------------------------------------------------- */

static void present_all(void)
{
    P->screen_present(0, 0, 0, 0);
}

static void fill_enc2_popup(void);

/* The selected RPK effect draws its own editor page, if it has one. */
static bool render_pack_screen(bool force)
{
    int rc;

    if (g_view != BOX_VIEW_EDITOR || !effect_is_rpk(g_selected_effect)) {
        g_pack_screen = false;
        return false;
    }
    force = force || g_pack_screen_effect != (int)g_selected_effect;
    rc = P->program_effect_screen_draw(g_selected_effect, force ? 1U : 0U);
    if (rc < 0) {
        g_pack_screen = false;
        if (g_screen_open) {
            /* The effect has no screen (any more): back to its tiles. */
            g_screen_open = false;
            g_own_screen = 0;
            g_view = BOX_VIEW_MAIN;
        }
        return false;
    }
    g_pack_screen = true;
    if (rc > 0) {
        force = true; /* a new frame */
    }
    if (force && g_preset_saved) {
        box_ui_render_saved_overlay(g_fb);
    }
    if (force) {
        present_all();
    }
    g_pack_screen_effect = g_selected_effect;
    return true;
}

static bool render_save_error(void)
{
    if (!g_save_error) return false;
    if ((int32_t)(now()-g_save_error_until) >= 0) { g_save_error=NULL; g_dirty=true; return false; }
    uint16_t *pixels; uint32_t stride;
    resona_ui *u=P->screen_begin(&pixels,&stride);
    u->clear(u->ctx,RESONA_RGB(7,9,12));
    u->text(u->ctx,14,42,2,RESONA_RGB(255,90,60),g_save_error);
    u->text(u->ctx,14,87,1,RESONA_RGB(232,227,211),"Check the programme or available space.");
    u->text(u->ctx,14,110,1,RESONA_RGB(110,120,125),"Select another preset to continue.");
    P->screen_present(0,0,320,172); return true;
}

static void preset_saved_show(void);
static void picker_open(void);
#include "program_ui.inc"

/* Keep the selected effect's tile inside the visible window. */
static void update_tile_window(void)
{
    uint8_t count = main_tile_count(); /* pedals and the EFFECT tile */

    if (count <= BOX_UI_VISIBLE_EFFECTS) {
        g_tile_first = 0U;
        return;
    }
    if (g_selected_main_item >= MAIN_EFFECT0) {
        uint8_t sel = (uint8_t)(g_selected_main_item - MAIN_EFFECT0);

        if (sel < g_tile_first) {
            g_tile_first = sel;
        } else if (sel >= g_tile_first + BOX_UI_VISIBLE_EFFECTS) {
            g_tile_first = (uint8_t)(sel - BOX_UI_VISIBLE_EFFECTS + 1U);
        }
    }
    if (g_tile_first > count - BOX_UI_VISIBLE_EFFECTS) {
        g_tile_first = (uint8_t)(count - BOX_UI_VISIBLE_EFFECTS);
    }
}

static void render_ui(void)
{
    box_ui_render_state_t state;
    box_ui_row_t rows[BOX_UI_MAX_ROWS];
    char values[BOX_UI_MAX_ROWS][16];
    static char preset_name[BOX_PRESET_NAME_LEN + 1U];
    resona_preset_info pinfo;
    uint32_t t = now();
    uint8_t count;

    if(program_draw_loading())return;
    sync();
    if (render_save_error()) return;
    if (render_pack_screen(true)) {
        g_meter_valid = false;
        g_dirty = false;
        return;
    }
    g_pack_screen_effect = -1;
    count = effect_count();
    update_tile_window();
    memset(&state, 0, sizeof(state));
    memset(rows, 0, sizeof(rows));
    memset(values, 0, sizeof(values));
    state.view = g_view;
    state.selected_main_item = g_selected_main_item;
    state.selected_effect = g_selected_effect;
    state.effect_count = count;
    state.effect_first = g_tile_first;
    state.effect_slot = g_view == BOX_VIEW_MAIN && effect_slot_shown();
    state.reorder_mode = g_reorder_mode;
    state.preset_browse = g_preset_browse;
    state.preset_save = g_preset_save;
    state.preset_conflict = g_preset_conflict;
    state.preset_text_visible = g_preset_blink_visible;
    state.preset_saved = g_preset_saved;
    state.editing = g_editing;
    state.global_eq_enabled = g_eq_enabled != 0;
    state.effects_bypass = g_bypass != 0;
    if (g_view == BOX_VIEW_MAIN && g_enc2_popup_until && (int32_t)(t - g_enc2_popup_until) < 0) {
        fill_enc2_popup();
        state.enc2_popup = true;
        state.enc2_popup_title = g_enc2_popup_title;
        state.enc2_popup_value = g_enc2_popup_value;
    }
    if (g_view == BOX_VIEW_MAIN && g_volume_popup_until && (int32_t)(t - g_volume_popup_until) < 0) {
        state.volume_popup = true;
    }
    state.master_volume = (uint8_t)clamp_int(g_volume, 0, 100);
    P->meters(&state.input_level, &state.output_level, NULL);
    memset(preset_name, 0, sizeof(preset_name));
    state.preset_index = (uint8_t)((g_preset_browse || g_preset_save || g_preset_target != g_preset_slot) ?
                                   g_preset_target : g_preset_slot);
    if (g_preset_slot >= 0 && state.preset_index == g_preset_slot && g_live_name[0]) {
        memcpy(preset_name, g_live_name, BOX_PRESET_NAME_LEN);
        state.preset_valid = true;
        state.preset_name = preset_name;
    } else if (state.preset_index < BOX_PRESET_COUNT &&
               P->preset_info(state.preset_index, &pinfo) == RESONA_OK && pinfo.used) {
        memcpy(preset_name, pinfo.name, BOX_PRESET_NAME_LEN);
        state.preset_valid = true;
        state.preset_name = preset_name[0] ? preset_name : "UNTITLED";
    } else if (state.preset_index < BOX_PRESET_COUNT &&
               (state.preset_index == g_preset_slot || g_preset_browse || g_preset_save ||
                g_preset_target != g_preset_slot)) {
        state.preset_valid = true;
        state.preset_name = "EMPTY";
    } else {
        state.preset_valid = false;
        state.preset_name = "INIT PRESET";
    }
    for (uint8_t i = 0U; i < count; ++i) {
        const resona_chain_effect *x = &g_chain.effects[i];

        state.effect_enabled[i] = x->enabled != 0;
        state.effect_tripped[i] = x->tripped != 0;
        state.effect_pack_name[i] = effect_builtin(i) ? NULL : effect_title(i);
        state.effect_art[i] = effect_builtin(i) ? (uint8_t)(x->engine - RESONA_ENGINE_GATE) : NONE;
    }

    if (g_view == BOX_VIEW_EDITOR) {
        uint8_t e = g_selected_effect;
        bool has_model = effect_has_model(e);
        uint8_t param_count;
        uint8_t first_item;
        uint8_t selected_item = 0U;
        uint8_t page_count;

        state.page_title = effect_title(e);
        state.selected_block_index = e;
        state.block_count = count;
        state.selector_is_model = has_model;
        param_count = build_editor_rows(e, rows, values);
        for (uint8_t i = 0U; i < param_count; ++i) {
            rows[i].mapped = effect_param_mapped(e, i);
        }
        if (g_selected_row >= param_count) {
            g_selected_row = 0U;
        }
        if (has_model) {
            int bank = model_bank(e);
            int idx = g_model_edit >= 0 ? g_model_edit : model_browse_origin(e);

            state.subtype_name = idx >= 0 ? g_model_names[bank][idx] : "EMPTY SLOT";
            state.subtype_index = idx >= 0 ? (uint8_t)idx : NONE;
            state.subtype_count = (uint8_t)g_model_count[bank];
        } else if (effect_engine(e) == RESONA_ENGINE_GATE) {
            state.subtype_name = g_chain.effects[e].variant == 1U ? "AUTO GATE" : "GATE";
            state.subtype_index = (uint8_t)(g_chain.effects[e].variant ? 1U : 0U);
            state.subtype_count = 2U;
        } else {
            state.subtype_name = effect_is_rpk(e) ? "USER PACK" : g_chain.effects[e].name;
            state.subtype_index = 0U;
            state.subtype_count = 1U;
        }
        page_count = param_count == 0U ? 1U : (uint8_t)((param_count + 2U) / 3U);
        if (g_editor_focus == BOX_EDITOR_FOCUS_MODEL) {
            selected_item = 0U;
            g_editor_page = 0U;
        } else if (g_editor_focus == BOX_EDITOR_FOCUS_PARAMETER) {
            selected_item = g_selected_row;
            g_editor_page = (uint8_t)(selected_item / 3U);
        } else if (g_editor_page >= page_count) {
            g_editor_page = 0U;
        }
        first_item = (uint8_t)(g_editor_page * 3U);
        state.rows = &rows[first_item];
        state.row_count = param_count > first_item ? (uint8_t)(param_count - first_item) : 0U;
        if (state.row_count > 3U) {
            state.row_count = 3U;
        }
        state.selected_row = selected_item >= first_item ? (uint8_t)(selected_item - first_item) : 0U;
        state.editor_focus = g_editor_focus;
        state.parameter_page = g_editor_page;
        state.parameter_page_count = page_count;
        state.editing = g_editing;
    } else if (g_view == BOX_VIEW_SETTINGS) {
        uint8_t total = build_settings_rows(rows, values);
        uint8_t first = (uint8_t)((g_settings_selected / 5U) * 5U);

        state.page_title = "SETTINGS";
        state.rows = &rows[first];
        state.row_count = (uint8_t)(total - first);
        if (state.row_count > 5U) {
            state.row_count = 5U;
        }
        state.selected_row = (uint8_t)(g_settings_selected - first);
        state.settings_index = g_settings_selected;
        state.settings_total = total;
        state.editing = g_editing;
    } else if (g_view == BOX_VIEW_PICKER) {
        uint8_t first = (uint8_t)((g_picker_selected / 5U) * 5U);
        uint8_t shown = (uint8_t)(g_picker_count - first);

        if (shown > 5U) {
            shown = 5U;
        }
        for (uint8_t i = 0U; i < shown; ++i) {
            const picker_item_t *it = &g_picker[first + i];
            bool current = (int)(first + i) == g_picker_current;

            /* The current choice reads CURRENT (in the fixed-value colour). */
            set_row(rows, i, it->name, current ? "CURRENT" : it->source == RESONA_CHAIN_RPK ? "RPK" : "",
                    !current);
        }
        state.page_title = "EFFECT";
        state.rows = rows;
        state.row_count = shown;
        state.selected_row = (uint8_t)(g_picker_selected - first);
        state.settings_index = g_picker_selected;
        state.settings_total = g_picker_count;
    } else if (g_view == BOX_VIEW_GLOBAL_EQ) {
        state.global_eq_selected = g_global_eq_selected;
        memcpy(state.global_eq_bands, g_eq_bands, sizeof(state.global_eq_bands));
        state.global_eq_level_steps = g_eq_level;
        state.editing = g_editing;
    }

    program_screen_probe(); /* before drawing the page: whatever the probe drew is covered */
    box_ui_render(g_fb, &state);
    if(program_controls_active()){
        program_controls_render();
        box_ui_render_main_overlay(g_fb,&state);
    }
    if (g_toast) {
        box_ui_render_toast(g_fb, "EFFECT", g_toast);
    }
    if (g_view == BOX_VIEW_MAIN) {
        g_main_state = state;
        g_main_state.rows = NULL;
        g_meter_in = state.input_level;
        g_meter_out = state.output_level;
        g_meter_valid = true;
    } else {
        g_meter_valid = false;
    }
    present_all();
    g_dirty = false;
}

/* ---- presets ------------------------------------------------------------------------- */

static void preset_blink_start(void)
{
    g_preset_blink_visible = true;
    g_preset_blink_next = now() + BLINK_MS;
    g_preset_saved = false;
    g_preset_saved_until = 0U;
}

static void preset_blink_stop(void)
{
    g_preset_blink_visible = true;
    g_preset_blink_next = 0U;
}

static void preset_saved_show(void)
{
    if(g_job.state>=RESONA_PROGRAM_QUEUED&&g_job.state<=RESONA_PROGRAM_STARTING)return;
    preset_blink_stop();
    g_preset_saved = true;
    g_preset_saved_until = now() + SAVED_MS;
}

static bool preset_has_data(int slot)
{
    resona_preset_info pi;

    return slot >= 0 && slot < (int)BOX_PRESET_COUNT && P->preset_info((uint32_t)slot, &pi) == RESONA_OK && pi.used;
}

static int apply_preset_preview(int slot){if(program_request(RESONA_PROGRAM_PREVIEW,(uint32_t)slot,NULL))return -1;g_preset_preview_valid=false;return 0;}

static void cancel_preset_preview(void)
{
    g_preset_apply_pending = -1;
    P->timer_stop(TIMER_PRESET);
}

static void schedule_preset_preview(int slot)
{
    g_preset_apply_pending = slot;
    P->timer_start(TIMER_PRESET, PRESET_APPLY_MS, 1U);
}

static int apply_pending_preset_preview(void)
{
    int slot = g_preset_apply_pending;

    if (slot >= 0) {
        cancel_preset_preview();
        return apply_preset_preview(slot);
    }
    if (g_preset_preview_valid && g_preset_preview_slot == g_preset_target) {
        return 0;
    }
    return apply_preset_preview(g_preset_target);
}

static int commit_preset(int slot){if(program_request(RESONA_PROGRAM_LOAD,(uint32_t)slot,NULL))return -1;g_preset_target=slot;return 0;}

static int save_preset(int slot){if(slot<0||slot>=(int)RESONA_PROGRAM_PRESETS)return -1;return program_request(RESONA_PROGRAM_SAVE,(uint32_t)slot,NULL);}

static void move_preset_target(int delta)
{
    uint8_t target;

    if (delta == 0) {
        return;
    }
    target = step_index_clamped((uint8_t)(g_preset_target < 0 ? 0 : g_preset_target), delta, BOX_PRESET_COUNT);
    if ((int)target == g_preset_target) {
        return;
    }
    g_preset_target = target;
    g_preset_conflict = false;
    if (g_preset_browse) {
        schedule_preset_preview(target);
    }
}

static void begin_preset_browse(void)
{
    cancel_preset_preview();
    g_preset_save = false;
    g_preset_conflict = false;
    g_reorder_mode = false;
    g_editing = false;
    g_preset_target = g_preset_slot >= 0 ? g_preset_slot : 0;
    g_preset_browse = true;
    g_preset_preview_slot = g_preset_slot;
    g_preset_preview_valid = g_preset_slot >= 0;
    g_preset_saved = false;
    g_preset_saved_until = 0U;
}

/* ---- navigation (as in the firmware UI) ------------------------------------------------ */

static void show_volume_popup(void)
{
    g_volume_popup_until = now() + POPUP_MS;
}

static void adjust_master_volume(int delta)
{
    if (set_volume(g_volume + delta * 2) == 0) {
        sync();
        show_volume_popup();
    }
}

/* Submit an edit (SET the preset's effect, MOVE a pedal) of the playing preset. It is a job:
 * the platform checks CPU and memory first; program_completion re-reads the chain and selects
 * effect `select_after`. Saving the preset stays the normal save. */
static bool submit_chain_edit(uint32_t op, uint32_t effect, uint32_t to, uint32_t source,
                              uint32_t entry, int select_after)
{
    resona_chain_edit edit;
    int id;

    if (g_edit_job || program_loading()) {
        show_toast("CHAIN BUSY");
        return false;
    }
    memset(&edit, 0, sizeof(edit));
    edit.version = 1U;
    edit.size = sizeof(edit);
    edit.op = op;
    edit.effect = effect;
    edit.to = to;
    edit.source = source;
    edit.entry = entry; /* engine stays 0: SET BUILTIN means the factory chain */
    id = P->program_edit(&edit);
    if (id <= 0) {
        show_toast(id == RESONA_ERR_UNSUPPORTED ? "NOT SUPPORTED" : chain_edit_error(id, op == RESONA_CHAIN_MOVE));
        return false;
    }
    g_requested_job = (uint32_t)id;
    g_edit_job = (uint32_t)id;
    g_edit_to = select_after;
    g_edit_keep_reorder = op == RESONA_CHAIN_MOVE;
    g_job.id = (uint32_t)id;
    g_job.action = 0U;
    g_job.state = RESONA_PROGRAM_QUEUED;
    g_dirty = true;
    return true;
}

/* Reorder mode: move the selected pedal along the factory chain. The move is a job; the
 * selection follows the pedal when it completes. */
static void reorder_selected_effect(int delta)
{
    uint8_t count = effect_count();
    int from, to;

    if (delta == 0 || g_edit_job || program_loading() || g_selected_main_item < MAIN_EFFECT0) {
        return;
    }
    from = g_selected_main_item - MAIN_EFFECT0;
    to = clamp_int(from + delta, 0, (int)count - 1);
    if (from >= (int)count || to == from) {
        return;
    }
    (void)submit_chain_edit(RESONA_CHAIN_MOVE, (uint32_t)from, (uint32_t)to, 0U, 0U, to);
}

/* ---- choosing the preset's effect ----------------------------------------------------- */

/* A stored file name as a player reads it: "02_velvet_comb.rpk" -> "velvet comb". Names stored
 * as the pack's own name pass through unchanged. */
static void picker_name(char *out, size_t cap, const char *name)
{
    const char *s = name, *end;
    size_t n = 0U;

    while (*s >= '0' && *s <= '9') ++s;
    if (s != name && (*s == '_' || *s == '-' || *s == ' ')) ++s; else s = name;
    end = s + strlen(s);
    for (const char *d = end; d > s; --d) {
        if (d[-1] == '.') { end = d - 1; break; }
    }
    if (end == s) { s = name; end = name + strlen(name); }
    for (; s < end && n + 1U < cap; ++s) out[n++] = *s == '_' ? ' ' : *s;
    out[n] = '\0';
}

static bool same_name(const char *a, const char *b)
{
    for (; *a && *b; ++a, ++b) {
        char x = *a >= 'a' && *a <= 'z' ? (char)(*a - 32) : *a;
        char y = *b >= 'a' && *b <= 'z' ? (char)(*b - 32) : *b;

        if (x != y) {
            return false;
        }
    }
    return *a == *b;
}

/* Is the preset playing this picker item? The chain names an RPK effect but not its entry,
 * so the RPK is matched by its cleaned name. */
static bool picker_is_current(const picker_item_t *it)
{
    char name[sizeof(it->name)];

    if (it->source == RESONA_CHAIN_BUILTIN) {
        return effect_count() > 0U && !chain_single_rpk();
    }
    if (!chain_single_rpk()) {
        return false;
    }
    picker_name(name, sizeof(name), g_chain.effects[0].name);
    return same_name(it->name, name);
}

/* The picker lists FACTORY CHAIN, then the RPK effects stored on the Box. */
static void picker_open(void)
{
    int n = P->entry_count();

    memset(g_picker, 0, sizeof(g_picker));
    g_picker[0].source = RESONA_CHAIN_BUILTIN;
    snprintf(g_picker[0].name, sizeof(g_picker[0].name), "FACTORY CHAIN");
    g_picker_count = 1U;
    for (int i = 0; i < n && g_picker_count < PICKER_MAX; ++i) {
        resona_entry_info en;
        picker_item_t *it;

        if (P->entry_info((uint32_t)i, &en) != RESONA_OK || en.kind != RESONA_ENTRY_PACK) {
            continue;
        }
        it = &g_picker[g_picker_count++];
        it->source = RESONA_CHAIN_RPK;
        it->entry = RESONA_ENTRY(RESONA_ENTRY_PACK, RESONA_ENTRY_INDEX(en.id));
        en.name[sizeof(en.name) - 1U] = '\0';
        if (en.name[0]) {
            picker_name(it->name, sizeof(it->name), en.name);
        } else {
            snprintf(it->name, sizeof(it->name), "RPK %u", (unsigned)RESONA_ENTRY_INDEX(en.id));
        }
    }
    g_picker_current = -1;
    for (uint8_t i = 0U; i < g_picker_count && g_picker_current < 0; ++i) {
        if (picker_is_current(&g_picker[i])) {
            g_picker_current = i;
        }
    }
    g_picker_selected = g_picker_current >= 0 ? (uint8_t)g_picker_current : 0U;
    g_reorder_mode = false;
    g_editing = false;
    g_view = BOX_VIEW_PICKER;
}

/* Play the chosen effect at its defaults; the current one just closes the picker. */
static void picker_choose(void)
{
    const picker_item_t *it;

    if (g_picker_selected >= g_picker_count) {
        return;
    }
    it = &g_picker[g_picker_selected];
    g_view = BOX_VIEW_MAIN;
    if ((int)g_picker_selected == g_picker_current) {
        return;
    }
    if (submit_chain_edit(RESONA_CHAIN_SET, 0U, 0U, it->source, it->entry, 0)) {
        g_main_param = 0;
    }
}

/* The picker owns Encoder 1: turn moves, click chooses, long press goes back. */
static void picker_input(const resona_event *e)
{
    if (e->input != RESONA_INPUT_SELECTOR) {
        return;
    }
    if (e->kind == RESONA_EVENT_TURN) {
        g_picker_selected = step_index_clamped(g_picker_selected, e->value, g_picker_count);
    } else if (e->kind == RESONA_EVENT_CLICK) {
        picker_choose();
    } else if (e->kind == RESONA_EVENT_LONG) {
        g_view = BOX_VIEW_MAIN;
    }
}

static void move_main(int delta)
{
    if (delta == 0) {
        return;
    }
    if (g_preset_browse || g_preset_save) {
        move_preset_target(delta);
        return;
    }
    if (program_controls_turn(delta)) {
        return;
    }
    if (g_reorder_mode) {
        reorder_selected_effect(delta);
        return;
    }
    if (g_selected_main_item == MAIN_VOLUME && g_editing) {
        adjust_master_volume(delta);
        return;
    }
    g_selected_main_item = step_index_clamped(g_selected_main_item, delta, (uint8_t)(MAIN_EFFECT0 + main_tile_count()));
    g_editing = false;
    if (g_selected_main_item >= MAIN_EFFECT0 && g_selected_main_item < MAIN_EFFECT0 + effect_count()) {
        g_selected_effect = (uint8_t)(g_selected_main_item - MAIN_EFFECT0);
    }
}

static void move_effect(int delta)
{
    uint8_t count = effect_count();
    uint8_t next;

    if (count == 0U || delta == 0) {
        return;
    }
    next = step_index_clamped(g_selected_effect, delta, count);
    g_selected_effect = next;
    g_selected_main_item = (uint8_t)(MAIN_EFFECT0 + next);
}

static void move_model(int delta)
{
    uint8_t e = g_selected_effect;
    int bank = model_bank(e);
    int n = bank >= 0 ? (int)g_model_count[bank] : 0;
    int next;

    if (delta == 0 || n == 0) {
        return;
    }
    next = clamp_int((g_model_edit < 0 ? (delta > 0 ? -1 : n) : g_model_edit) + delta, 0, n - 1);
    if (next == g_model_edit) {
        return;
    }
    int rc = P->program_effect_model(e, RESONA_ENTRY_INDEX(g_model_ids[bank][next]));
    if (rc == RESONA_OK) {
        g_model_edit = next;
        sync();
    } else {
        g_model_edit = model_selected(e);
    }
}

static uint8_t editor_row_count(void)
{
    box_ui_row_t rows[BOX_UI_MAX_ROWS];
    char values[BOX_UI_MAX_ROWS][16];

    if (effect_builtin(g_selected_effect)) {
        return effect_row_count(g_selected_effect);
    }
    return build_editor_rows(g_selected_effect, rows, values);
}

static bool editor_row_mutable(void)
{
    return g_selected_row < editor_row_count();
}

static void move_subtype(int delta)
{
    uint8_t e = g_selected_effect;
    uint8_t count = effect_variant_count(e);
    uint8_t next;

    if (delta == 0 || count <= 1U) {
        return;
    }
    next = step_index_clamped((uint8_t)g_chain.effects[e].variant, delta, count);
    if (next != g_chain.effects[e].variant) {
        (void)P->program_effect_variant(e, next);
    }
}

static void move_editor_focus(int delta)
{
    bool has_model = effect_has_model(g_selected_effect);
    uint8_t item_count = (uint8_t)(3U + editor_row_count());
    int current;
    uint8_t next;

    if (delta == 0) {
        return;
    }
    if (g_editor_focus == BOX_EDITOR_FOCUS_BLOCK) {
        current = 0;
    } else if (g_editor_focus == BOX_EDITOR_FOCUS_STATE) {
        current = 1;
    } else if (g_editor_focus == BOX_EDITOR_FOCUS_TYPE ||
               (g_editor_focus == BOX_EDITOR_FOCUS_MODEL && has_model)) {
        current = 2;
    } else {
        current = 3 + (int)g_selected_row;
    }
    next = step_index_clamped((uint8_t)current, delta, item_count);
    if (next == 0) {
        g_editor_focus = BOX_EDITOR_FOCUS_BLOCK;
        g_selected_row = 0U;
    } else if (next == 1) {
        g_editor_focus = BOX_EDITOR_FOCUS_STATE;
        g_selected_row = 0U;
    } else if (next == 2) {
        g_editor_focus = has_model ? BOX_EDITOR_FOCUS_MODEL : BOX_EDITOR_FOCUS_TYPE;
        g_selected_row = 0U;
        g_editor_page = 0U;
    } else {
        g_editor_focus = BOX_EDITOR_FOCUS_PARAMETER;
        g_selected_row = (uint8_t)(next - 3);
        g_editor_page = (uint8_t)(g_selected_row / 3U);
    }
}

/* One detent on parameter `row` of effect e: built-ins move in their display steps. */
static void edit_effect_value(uint8_t e, uint8_t row, int delta)
{
    uint32_t engine = effect_engine(e);
    int value;

    if (delta == 0 || e >= effect_count()) {
        return;
    }
    if (!effect_builtin(e)) {
        nudge_effect_param(e, row, delta);
        return;
    }
    if (row >= effect_row_count(e)) {
        return;
    }
    if (engine == RESONA_ENGINE_CAB && row == 2U) {
        /* Off-grid tenths move to the next half-dB in the turn direction. */
        int steps = (cab_level_tenths(e) + 120 + (delta < 0 ? 4 : 0)) / 5;

        value = clamp_int(steps + delta, 0, 48) * 5 - 120;
        param_norm_set(e, row, (float)(value + 120) / 240.0f);
    } else {
        value = clamp_int((int)param_percent(e, row) + delta, 0, 100);
        param_norm_set(e, row, (float)value / 100.0f);
    }
    g_dirty = true;
}

static void edit_editor_value(int delta)
{
    if (delta == 0 || !editor_row_mutable()) {
        return;
    }
    edit_effect_value(g_selected_effect, g_selected_row, delta);
    sync();
}

static void edit_editor_selection(int delta)
{
    if (g_editor_focus == BOX_EDITOR_FOCUS_BLOCK) {
        move_effect(delta);
        g_model_edit = model_browse_origin(g_selected_effect);
        g_selected_row = 0U;
        g_editor_page = 0U;
    } else if (g_editor_focus == BOX_EDITOR_FOCUS_STATE) {
        if (delta != 0 && effect_enabled(g_selected_effect) != (delta > 0)) {
            toggle_effect(g_selected_effect);
        }
    } else if (g_editor_focus == BOX_EDITOR_FOCUS_TYPE) {
        move_subtype(delta);
    } else if (g_editor_focus == BOX_EDITOR_FOCUS_MODEL) {
        move_model(delta);
    } else {
        edit_editor_value(delta);
    }
}

static void edit_mapped_value(int delta)
{
    if (delta == 0) {
        return;
    }
    if (eq_mapped()) {
        if (g_enc2_map_row >= 1U && g_enc2_map_row <= BOX_UI_GLOBAL_EQ_BANDS) {
            uint8_t band = (uint8_t)(g_enc2_map_row - 1U);

            g_eq_bands[band] = (int8_t)clamp_int(g_eq_bands[band] + delta, -30, 30);
        } else if (g_enc2_map_row == BOX_UI_GLOBAL_EQ_BANDS + 1U) {
            g_eq_level = (int8_t)clamp_int(g_eq_level + delta, -24, 24);
        }
        eq_apply(true);
        return;
    }
    if (effect_mapped()) {
        edit_effect_value((uint8_t)g_program.encoder_effect, (uint8_t)g_program.encoder_parameter, delta);
    }
}

static void move_global_eq(int delta)
{
    if (delta == 0) {
        return;
    }
    if (g_editing) {
        if (g_global_eq_selected >= 1U && g_global_eq_selected <= BOX_UI_GLOBAL_EQ_BANDS) {
            uint8_t band = (uint8_t)(g_global_eq_selected - 1U);

            g_eq_bands[band] = (int8_t)clamp_int(g_eq_bands[band] + delta, -30, 30);
        } else if (g_global_eq_selected == BOX_UI_GLOBAL_EQ_BANDS + 1U) {
            g_eq_level = (int8_t)clamp_int(g_eq_level + delta, -24, 24);
        }
        eq_apply(false); /* heard at once, stored on confirm */
        return;
    }
    g_global_eq_selected = step_index_clamped(g_global_eq_selected, delta, BOX_UI_GLOBAL_EQ_BANDS + 2U);
}

static void sync_input_policy(void)
{
    P->input_config(RESONA_INPUT_CUSTOMIZE,
                    (g_view == BOX_VIEW_MAIN && g_gesture_actions[GESTURE_LONG] == ACTION_NONE) ?
                    RESONA_INPUT_PRESS_ON_DOWN : 0U);
    /* No double click (and so no click delay) while browsing or picking. */
    P->input_config(RESONA_INPUT_SELECTOR, (g_preset_browse || g_preset_save ||
                                            g_view == BOX_VIEW_PICKER) ? 0U : RESONA_INPUT_DOUBLE);
}

static void edit_setting(int delta)
{
    int32_t v = 0;

    if (delta == 0) {
        return;
    }
    switch (g_settings_selected) {
    case SETTING_LOOPBACK:
        P->setting_set(RESONA_SETTING_LOOPBACK, delta > 0);
        break;
    case SETTING_INPUT_GAIN:
        P->setting_get(RESONA_SETTING_INPUT_GAIN, &v);
        P->setting_set(RESONA_SETTING_INPUT_GAIN, clamp_int((int)v + delta, -20, 20));
        break;
    case SETTING_USB_VOLUME:
        P->setting_get(RESONA_SETTING_USB_VOLUME, &v);
        P->setting_set(RESONA_SETTING_USB_VOLUME, clamp_int((int)v + delta, 0, 100));
        break;
    case SETTING_BLUETOOTH:
        P->setting_set(RESONA_SETTING_BT_ENABLE, delta > 0);
        break;
    case SETTING_BT_VOLUME:
        P->setting_get(RESONA_SETTING_BT_VOLUME, &v);
        P->setting_set(RESONA_SETTING_BT_VOLUME, clamp_int((int)v + delta, 0, 100));
        break;
    case SETTING_MIDI_CHANNEL:
        P->setting_get(RESONA_SETTING_MIDI_CHANNEL, &v);
        P->setting_set(RESONA_SETTING_MIDI_CHANNEL, clamp_int((int)v + delta, 0, 16));
        break;
    case SETTING_ENC2_PRESS:
    case SETTING_ENC2_LONG: {
        uint8_t g = (uint8_t)(g_settings_selected - SETTING_ENC2_PRESS);

        g_gesture_actions[g] = step_index_clamped(g_gesture_actions[g], delta, ACTION_COUNT);
        store_save();
        sync_input_policy();
        break;
    }
    default:
        break;
    }
}

static void open_selected_block(void)
{
    if (g_selected_effect >= effect_count()) {
        return;
    }
    g_selected_row = 0U;
    g_editor_focus = BOX_EDITOR_FOCUS_BLOCK;
    g_editor_page = 0U;
    g_editing = false;
    g_model_edit = model_browse_origin(g_selected_effect);
    g_view = BOX_VIEW_EDITOR;
}

static void primary_action(void)
{
    switch (g_view) {
    case BOX_VIEW_MAIN:
        if (g_reorder_mode) {
            g_reorder_mode = false;
            break;
        }
        if (g_preset_save) {
            if (preset_has_data(g_preset_target) && !g_preset_conflict) {
                g_preset_conflict = true;
            } else if (save_preset(g_preset_target) == 0) {
                g_preset_save = false;
                g_preset_conflict = false;
                preset_saved_show();
            }
            break;
        }
        if (g_preset_browse) {
            if (apply_pending_preset_preview() == 0 && commit_preset(g_preset_target) == 0) {
                g_preset_browse = false;
            }
            break;
        }
        switch (g_selected_main_item) {
        case MAIN_PRESET:
            begin_preset_browse();
            break;
        case MAIN_GLOBAL:
            g_view = BOX_VIEW_SETTINGS;
            g_editing = false;
            break;
        case MAIN_VOLUME:
            g_editing = !g_editing;
            if (g_editing) {
                show_volume_popup();
            } else {
                g_volume_popup_until = 0U;
            }
            break;
        default:
            if (program_controls_active()) {
                program_controls_click();
                break;
            }
            if (effect_slot_selected()) {
                picker_open();
                break;
            }
            open_selected_block();
            break;
        }
        break;
    case BOX_VIEW_EDITOR:
        if (g_editor_focus == BOX_EDITOR_FOCUS_BLOCK || g_editor_focus == BOX_EDITOR_FOCUS_STATE) {
            g_editing = !g_editing;
        } else if (g_editor_focus == BOX_EDITOR_FOCUS_TYPE && effect_variant_count(g_selected_effect) > 1U) {
            g_editing = !g_editing;
        } else if (g_editor_focus == BOX_EDITOR_FOCUS_MODEL && effect_has_model(g_selected_effect)) {
            if (!g_editing && g_model_edit < 0) {
                g_model_edit = model_browse_origin(g_selected_effect);
            }
            g_editing = !g_editing;
        } else if (g_editor_focus == BOX_EDITOR_FOCUS_PARAMETER && editor_row_mutable()) {
            g_editing = !g_editing;
        }
        break;
    case BOX_VIEW_SETTINGS:
        if (g_settings_selected == SETTING_GLOBAL_EQ) {
            g_view = BOX_VIEW_GLOBAL_EQ;
            g_editing = false;
        } else if (g_settings_selected != SETTING_FIRMWARE) {
            g_editing = !g_editing;
        }
        break;
    case BOX_VIEW_GLOBAL_EQ:
        if (g_global_eq_selected == 0U) {
            g_eq_enabled = !g_eq_enabled;
            eq_apply(true);
            g_editing = false;
        } else if (g_editing) {
            eq_apply(true);
            g_editing = false;
        } else {
            g_editing = true;
        }
        break;
    default:
        break;
    }
}

static void save_action(void)
{
    int slot = g_preset_slot >= 0 ? g_preset_slot : 0;

    if (g_preset_browse || g_preset_save || g_view == BOX_VIEW_PICKER) {
        return;
    }
    if (g_view == BOX_VIEW_MAIN && g_selected_main_item == MAIN_PRESET) {
        g_preset_save = true;
        g_preset_conflict = false;
        g_preset_target = slot;
        preset_blink_start();
        return;
    }
    if (save_preset(slot) == 0) {
        preset_saved_show();
    }
}

static void home_action(void)
{
    if (g_view == BOX_VIEW_GLOBAL_EQ) {
        eq_apply(true);
    }
    cancel_preset_preview();
    if (g_preset_browse && g_preset_preview_valid && g_preset_preview_slot != g_preset_slot &&
        g_preset_slot >= 0) {
        /* Browsing was only listening: go back to what was playing. */
        (void)program_request(RESONA_PROGRAM_PREVIEW, (uint32_t)g_preset_slot, NULL);
    }
    g_view = BOX_VIEW_MAIN;
    g_screen_open = false;
    g_editing = false;
    g_reorder_mode = false;
    g_preset_browse = false;
    g_preset_save = false;
    g_preset_conflict = false;
    g_preset_target = g_preset_slot;
    g_model_edit = -1;
    g_volume_popup_until = 0U;
    preset_blink_stop();
}

static void step_preset(int delta)
{
    int candidate;
    int direction;

    if (delta == 0) {
        return;
    }
    if (g_view != BOX_VIEW_MAIN) {
        home_action();
    }
    cancel_preset_preview();
    direction = delta > 0 ? 1 : -1;
    candidate = g_preset_slot >= 0 ? clamp_int(g_preset_slot + direction, 0, BOX_PRESET_COUNT - 1) :
                (direction > 0 ? 0 : (int)BOX_PRESET_COUNT - 1);
    if (candidate == g_preset_slot) {
        return;
    }
    if (commit_preset(candidate) == 0) {
        g_preset_browse = false;
        g_preset_save = false;
        g_preset_conflict = false;
    }
}

static void encoder1_long_action(void)
{
    bool on_pedal = g_selected_main_item >= MAIN_EFFECT0 && g_selected_main_item < MAIN_EFFECT0 + effect_count();

    /* Long press on a pedal of the factory chain starts moving it; anywhere else (and while
     * moving) it goes home. A one-RPK preset has nothing to reorder. */
    if (g_view == BOX_VIEW_MAIN && !g_preset_browse && !g_preset_save && on_pedal &&
        !program_controls_active() && !g_reorder_mode) {
        g_reorder_mode = true;
        g_editing = false;
        return;
    }
    if (program_controls_long()) {
        return;
    }
    home_action();
}

static bool assign_enc2_from_view(void)
{
    if (g_view == BOX_VIEW_EDITOR && g_editor_focus == BOX_EDITOR_FOCUS_PARAMETER && editor_row_mutable()) {
        if (P->program_map_encoder(g_selected_effect, g_selected_row) != RESONA_OK) {
            return false;
        }
        g_enc2_map_valid = false; /* the effect parameter replaces a Global EQ mapping */
        store_save();
        sync();
        return true;
    }
    if (g_view == BOX_VIEW_GLOBAL_EQ && g_global_eq_selected >= 1U) {
        (void)P->program_map_encoder(RESONA_PROGRAM_NO_PARAMETER, RESONA_PROGRAM_NO_PARAMETER);
        g_enc2_map_valid = true;
        g_enc2_map_kind = MAP_GLOBAL_EQ;
        g_enc2_map_row = g_global_eq_selected;
        store_save();
        return true;
    }
    return false;
}

static bool current_control_is_enc2_mapped(void)
{
    if (g_view == BOX_VIEW_EDITOR) {
        return g_editor_focus == BOX_EDITOR_FOCUS_PARAMETER &&
               effect_param_mapped(g_selected_effect, g_selected_row);
    }
    if (g_view == BOX_VIEW_GLOBAL_EQ) {
        return eq_mapped() && g_global_eq_selected >= 1U && g_enc2_map_row == g_global_eq_selected;
    }
    return false;
}

static bool clear_enc2_mapping(void)
{
    if (g_view == BOX_VIEW_EDITOR) {
        (void)P->program_map_encoder(RESONA_PROGRAM_NO_PARAMETER, RESONA_PROGRAM_NO_PARAMETER);
        sync();
    } else {
        g_enc2_map_valid = false;
        g_enc2_map_kind = MAP_EFFECT;
        g_enc2_map_row = 0U;
        store_save();
    }
    g_enc2_popup_until = 0U;
    return true;
}

static void fill_enc2_popup(void)
{
    static const char *const eq_names[BOX_UI_GLOBAL_EQ_BANDS + 1U] = {"100", "250", "630", "1.6K", "4K", "LEVEL"};
    box_ui_row_t rows[BOX_UI_MAX_ROWS];
    char values[BOX_UI_MAX_ROWS][16];
    uint8_t count, e, row;

    memset(g_enc2_popup_title, 0, sizeof(g_enc2_popup_title));
    memset(g_enc2_popup_value, 0, sizeof(g_enc2_popup_value));
    if (eq_mapped()) {
        uint8_t control = g_enc2_map_row;

        if (control < 1U || control > BOX_UI_GLOBAL_EQ_BANDS + 1U) {
            return;
        }
        snprintf(g_enc2_popup_title, sizeof(g_enc2_popup_title), "GLOBAL EQ %s", eq_names[control - 1U]);
        format_eq_steps(g_enc2_popup_value, control <= BOX_UI_GLOBAL_EQ_BANDS ? g_eq_bands[control - 1U] : g_eq_level);
        return;
    }
    if (!effect_mapped()) {
        return;
    }
    e = (uint8_t)g_program.encoder_effect;
    row = (uint8_t)(g_program.encoder_parameter < BOX_UI_MAX_ROWS ? g_program.encoder_parameter : NONE);
    memset(rows, 0, sizeof(rows));
    count = build_editor_rows(e, rows, values);
    if (row >= count) {
        return;
    }
    if (effect_count() == 1U && effect_is_rpk(0U)) {
        snprintf(g_enc2_popup_title, sizeof(g_enc2_popup_title), "%s", rows[row].label);
    } else {
        snprintf(g_enc2_popup_title, sizeof(g_enc2_popup_title), "%s %s", effect_title(e), rows[row].label);
    }
    snprintf(g_enc2_popup_value, sizeof(g_enc2_popup_value), "%s", rows[row].value);
}

/* Switch the first effect in the chain that runs `engine` on or off. */
static void toggle_engine(uint32_t engine)
{
    for (uint8_t e = 0U; e < effect_count(); ++e) {
        if (effect_engine(e) == engine) {
            toggle_effect(e);
            return;
        }
    }
}

static void perform_action(uint8_t action)
{
    switch (action) {
    case ACTION_PRESET_UP: step_preset(1); break;
    case ACTION_PRESET_DOWN: step_preset(-1); break;
    case ACTION_GATE:
    case ACTION_COMP:
    case ACTION_RESONA:
    case ACTION_NAM:
    case ACTION_CAB: toggle_engine(RESONA_ENGINE_GATE + (uint32_t)(action - ACTION_GATE)); break;
    case ACTION_BYPASS: P->setting_set(RESONA_SETTING_BYPASS, g_bypass ? 0 : 1); break;
    case ACTION_GLOBAL_EQ:
        g_eq_enabled = !g_eq_enabled;
        eq_apply(true);
        break;
    default: break;
    }
}

static void led_pulse(uint8_t gesture)
{
    g_led_pulse = gesture == GESTURE_LONG ? RESONA_LED_PULSE_LONG : RESONA_LED_PULSE;
    g_led_until = now() + LED_PULSE_MS;
}

static void poll_led(void)
{
    uint32_t t = now();

    if (g_bypass) {
        P->led_set(((t / LED_BYPASS_FLASH_MS) & 1U) ? RESONA_LED_BYPASS : RESONA_LED_OFF);
        return;
    }
    if (g_led_pulse && (int32_t)(t - g_led_until) < 0) {
        P->led_set(g_led_pulse);
        return;
    }
    g_led_pulse = 0U;
    P->led_set((g_editing || g_preset_browse || g_preset_save || g_reorder_mode || g_view == BOX_VIEW_EDITOR) ?
               RESONA_LED_EDIT : RESONA_LED_HOVER);
}

/* ---- input ----------------------------------------------------------------------------- */

/* On an RPK effect's own screen both encoders belong to the effect, except Encoder 1 long
 * press (home) and double press (save). */
static bool pack_screen_input(const resona_event *e)
{
    if (!g_pack_screen || g_view != BOX_VIEW_EDITOR || !effect_is_rpk(g_selected_effect)) {
        return false;
    }
    if (e->input == RESONA_INPUT_SELECTOR && (e->kind == RESONA_EVENT_LONG || e->kind == RESONA_EVENT_DOUBLE)) {
        return false;
    }
    P->program_effect_screen_event(g_selected_effect, e);
    return true;
}

static void handle_input(const resona_event *e)
{
    if (g_toast && e->kind != RESONA_EVENT_TURN) {
        g_toast = NULL; /* any press dismisses the toast */
    }
    if (g_view == BOX_VIEW_PICKER) {
        picker_input(e);
        return;
    }
    if (pack_screen_input(e)) {
        return;
    }
    if (e->kind == RESONA_EVENT_TURN) {
        if (e->input == RESONA_INPUT_SELECTOR) {
            if (g_view == BOX_VIEW_MAIN) {
                move_main(e->value);
            } else if (g_view == BOX_VIEW_EDITOR) {
                if (g_editing) {
                    edit_editor_selection(e->value);
                } else {
                    move_editor_focus(e->value);
                }
            } else if (g_view == BOX_VIEW_SETTINGS) {
                if (g_editing) {
                    edit_setting(e->value);
                } else {
                    g_settings_selected = step_index_clamped(g_settings_selected, e->value, SETTING_COUNT);
                }
            } else if (g_view == BOX_VIEW_GLOBAL_EQ) {
                move_global_eq(e->value);
            }
        } else {
            edit_mapped_value(e->value);
            if (g_view == BOX_VIEW_MAIN) {
                fill_enc2_popup();
                g_enc2_popup_until = now() + POPUP_MS;
            }
        }
    } else if (e->kind == RESONA_EVENT_CLICK || e->kind == RESONA_EVENT_DOUBLE) {
        if (e->input == RESONA_INPUT_SELECTOR) {
            if (e->kind == RESONA_EVENT_DOUBLE) {
                save_action();
            } else {
                primary_action();
            }
        } else if (g_view == BOX_VIEW_MAIN && e->kind == RESONA_EVENT_CLICK) {
            if (g_gesture_actions[GESTURE_PRESS] != ACTION_BYPASS) {
                led_pulse(GESTURE_PRESS);
            }
            perform_action(g_gesture_actions[GESTURE_PRESS]);
        }
    } else if (e->kind == RESONA_EVENT_LONG) {
        if (e->input == RESONA_INPUT_SELECTOR) {
            encoder1_long_action();
        } else if (program_controls_map()) {
            led_pulse(GESTURE_LONG);
        } else if (current_control_is_enc2_mapped()) {
            if (clear_enc2_mapping()) {
                led_pulse(GESTURE_LONG);
            }
        } else if (assign_enc2_from_view()) {
            led_pulse(GESTURE_LONG);
        } else if (g_view == BOX_VIEW_MAIN) {
            if (g_gesture_actions[GESTURE_LONG] != ACTION_BYPASS) {
                led_pulse(GESTURE_LONG);
            }
            perform_action(g_gesture_actions[GESTURE_LONG]);
        }
    }
}

/* ---- the app ------------------------------------------------------------------------------ */

static int app_init(void *state, const resona_platform *platform)
{
    (void)state;
    P = platform;
    if (P->version != RESONA_PLATFORM_VERSION || P->size < sizeof(resona_platform)) {
        return -1;
    }
    P->device_info(&g_dev);
    P->screen_begin(&g_fb, &g_stride);
    if (!g_fb || g_stride != BOX_UI_WIDTH) {
        return -1;
    }
    store_load();
    scan_models();
    sync();
    g_view = BOX_VIEW_MAIN;
    g_selected_main_item = MAIN_PRESET;
    g_selected_effect = 0U;
    g_editor_focus = BOX_EDITOR_FOCUS_BLOCK;
    g_settings_selected = SETTING_GLOBAL_EQ;
    g_global_eq_selected = 3U;
    g_preset_target = g_preset_slot;
    g_preset_preview_slot = g_preset_slot;
    g_preset_preview_valid = g_preset_slot >= 0;
    g_model_edit = -1;
    sync_input_policy();
    P->log("resona factory app: ready");
    render_ui();
    return 0;
}

static void app_event(void *state, const resona_app_event *ev)
{
    (void)state;
    switch (ev->kind) {
    case RESONA_APP_EV_INPUT:
        handle_input(&ev->u.input);
        sync_input_policy();
        g_dirty = true;
        break;
    case RESONA_APP_EV_TIMER:
        if (ev->u.timer.id == TIMER_PRESET && g_preset_apply_pending >= 0) {
            int slot = g_preset_apply_pending;

            g_preset_apply_pending = -1;
            apply_preset_preview(slot);
            g_dirty = true;
        }
        break;
    case RESONA_APP_EV_AUDIO: {
        char line[48];

        /* The OS no longer reports a role here (tripped_role is always -1). */
        snprintf(line, sizeof(line), "audio at %u frames", (unsigned)ev->u.audio.block_frames);
        P->log(line);
        g_dirty = true;
        break;
    }
    case RESONA_APP_EV_CHANGED:
        if (ev->u.changed.what == RESONA_CHANGED_MODELS) {
            scan_models();
        }
        if (ev->u.changed.what == RESONA_CHANGED_PROGRAM_JOB) {
            program_completion();
        }
        if (ev->u.changed.what == RESONA_CHANGED_CHAIN) {
            /* Effect indices changed: a model being browsed may belong to another effect.
             * sync() clamps the selection to the new chain. */
            g_model_edit = -1;
        }
        if (ev->u.changed.what == RESONA_CHANGED_PRESET) {
            g_own_screen = -1; /* a new load: ask the effect again */
        }
        if (ev->u.changed.what == RESONA_CHANGED_PRESET && !program_loading()) {
            /* A committed recall (including deferred boot recall) replaces
             * the browse target as well as the live state. */
            sync();
            g_preset_target = g_preset_slot;
            g_preset_preview_slot = g_preset_slot;
            g_preset_preview_valid = g_preset_slot >= 0;
            P->timer_stop(TIMER_PRESET);
            g_preset_browse = false;
            g_preset_save = false;
            g_preset_conflict = false;
            g_preset_apply_pending = -1;
        }
        g_dirty = true;
        break;
    default:
        break;
    }
}

static void app_frame(void *state, uint32_t ms)
{
    uint32_t t = now();
    bool controls_changed = false;

    (void)state;
    (void)ms;
    program_completion();
    poll_led();
    if(program_draw_loading())return;
    if (render_save_error()) return;
    if (g_toast && (int32_t)(t - g_toast_until) >= 0) {
        g_toast = NULL;
        g_dirty = true;
    }
    if (g_view == BOX_VIEW_MAIN && t - g_chain_poll_ms >= 250U) {
        resona_chain_view chain;

        g_chain_poll_ms = t;
        if (!program_loading() && read_chain(&chain) && memcmp(&chain, &g_chain, sizeof(chain)) != 0) {
            g_dirty = true;
        }
    }
    if (g_preset_saved && g_preset_saved_until && (int32_t)(t - g_preset_saved_until) >= 0) {
        g_preset_saved = false;
        g_preset_saved_until = 0U;
        g_dirty = true;
    }
    if (g_view == BOX_VIEW_MAIN && g_enc2_popup_until && (int32_t)(t - g_enc2_popup_until) >= 0) {
        g_enc2_popup_until = 0U;
        g_dirty = true;
    }
    if (g_view == BOX_VIEW_MAIN && g_volume_popup_until && (int32_t)(t - g_volume_popup_until) >= 0) {
        g_volume_popup_until = 0U;
        g_dirty = true;
    }
    if (g_dirty) {
        render_ui();
        return;
    }
    if (g_pack_screen) {
        render_pack_screen(false); /* follows the effect's state; an unchanged frame is not redrawn */
        return;
    }
    if (g_view != BOX_VIEW_MAIN) {
        return;
    }
    if (g_preset_save && g_preset_blink_next && (int32_t)(t - g_preset_blink_next) >= 0) {
        g_preset_blink_visible = !g_preset_blink_visible;
        g_preset_blink_next = t + BLINK_MS;
        controls_changed = true;
    } else if (!g_preset_save && !g_preset_blink_visible) {
        preset_blink_stop();
        controls_changed = true;
    }
    if (controls_changed) {
        g_main_state.preset_save = g_preset_save;
        g_main_state.preset_conflict = g_preset_conflict;
        g_main_state.preset_text_visible = g_preset_blink_visible;
        g_main_state.preset_saved = g_preset_saved;
        box_ui_render_main_controls(g_fb, &g_main_state);
        P->screen_present(0, 28, BOX_UI_WIDTH, 33);
    }
    {
        uint8_t in = 0, out = 0;

        P->meters(&in, &out, NULL);
        if (!g_meter_valid || in != g_meter_in || out != g_meter_out) {
            box_ui_render_meters(g_fb, in, out);
            g_meter_in = in;
            g_meter_out = out;
            g_meter_valid = true;
            P->screen_present(38, 8, 270, 11);
        }
    }
}

static const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    RESONA_APP_ID, RESONA_APP_NAME, RESONA_APP_VERSION, /* from resona.project.json */
    0U, 0U,
    app_init, app_event, app_frame,
};

const resona_app *resona_app_get(void)
{
    return &kApp;
}
