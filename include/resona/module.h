/*
 * Resona module API, ABI 2 (development).
 *
 * A module is one shared library (host build) or one relocatable image inside a
 * pack (device build, see the SDK's pack guide) that exports `resona_module_get`. The
 * host owns all memory: it allocates the state block you declare and passes it
 * to every call, so a module never needs the heap.
 *
 * One module runs on every device of the family. Nothing about the device is
 * fixed at compile time except the limits below: the sample rate, the largest
 * block, the channel layout, the screen size and the number of encoders come
 * from the host. A module lists the sample rates and layouts it supports; a host
 * refuses a module it cannot run.
 *
 * Audio contract: 32-bit float samples, nominally in [-1, 1], non-interleaved,
 * one pointer per channel. process() gets 1 to host->max_block_frames frames.
 * `in` and `out` may point at the same buffers (in-place processing).
 *
 * Real-time rules for process():
 *   - no malloc/free/new/delete, no exceptions, no RTTI, no file or console I/O;
 *   - no unbounded loops; the worst case must fit the block deadline;
 *   - no transcendental libm calls (sinf, powf, expf, ...); compute
 *     coefficients in set_param() or init() instead;
 *   - never produce NaN or infinity; flush denormals to zero in feedback paths.
 * On current Resona firmware set_param() is coalesced and called immediately
 * before process(), within the same block deadline; keep coefficient work bounded.
 * Older hosts may call set_param() during process(). For portable code, and for
 * UI on_event() edits, use resona/parameters.h: publish values from control and
 * apply them at the start of process(). Never mutate DSP coefficients in ui_draw().
 *
 * GPL-3.0 licensed, see LICENSE.
 */
#ifndef RESONA_MODULE_H
#define RESONA_MODULE_H

#include <stdint.h>
#include "code.h"
#include <stddef.h>
#include <resona/memory.h>
#include <resona/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RESONA_MODULE_ABI 2u
#define RESONA_MAX_PARAMS 12u /* legacy UI/batch width; ABI2 parameter count is resource-bounded */
#define RESONA_STATE_ALIGN 16u
/* Absolute declaration sanity bound, not a per-instance reservation. Actual
 * admission uses shared live SRAM/PSRAM budgets, including runtime overhead. */
#define RESONA_MAX_STATE_BYTES 4194304u

/* Size buffers (delay lines, tables) for these: no host exceeds them. */
#define RESONA_MAX_SAMPLE_RATE 48000u
#define RESONA_MAX_BLOCK_FRAMES 256u
#define RESONA_MAX_CHANNELS 2u

/* Sample rates a module supports (resona_module.rates). */
#define RESONA_RATE_44100 0x1u
#define RESONA_RATE_48000 0x2u

/* Channel layouts a module supports (resona_module.layouts). */
#define RESONA_LAYOUT_MONO 0x1u   /* one input, one output */
#define RESONA_LAYOUT_STEREO 0x2u /* two inputs, two outputs */

#if defined(_WIN32)
#define RESONA_EXPORT __declspec(dllexport)
#else
#define RESONA_EXPORT __attribute__((visibility("default")))
#endif

/* One user-visible parameter. Values are always in [min, max]. */
typedef struct resona_param {
    const char *name; /* at most 12 characters; shown on the device */
    float min;
    float max;
    float def;        /* default value */
    const char *unit; /* "dB", "Hz", "ms", "%" or "" */
} resona_param;

/*
 * Optional details of a parameter, used by the host's own parameter page and by
 * encoders: how a value moves and how it is shown. A module without them gets a
 * continuous, linear control for every parameter.
 */
#define RESONA_PARAM_STEPPED 0x1u /* moves in units of `step` from min */
#define RESONA_PARAM_TOGGLE 0x2u  /* off (min) or on (max) */
#define RESONA_PARAM_CHOICE 0x4u  /* integer index min..max shown as choices[index - min] */
#define RESONA_PARAM_LOG 0x8u     /* logarithmic taper (min > 0), e.g. frequencies */

typedef struct resona_param_ext {
    uint32_t flags;              /* RESONA_PARAM_* */
    float step;                  /* for RESONA_PARAM_STEPPED; 0 otherwise */
    uint32_t choice_count;       /* for RESONA_PARAM_CHOICE: max - min + 1 */
    const char *const *choices;  /* for RESONA_PARAM_CHOICE; at most 12 characters each */
} resona_param_ext;

/* What the host tells the module at init time. */
typedef struct resona_host {
    uint32_t abi;
    uint32_t sample_rate;      /* one of the rates the module listed */
    uint32_t max_block_frames; /* process() never gets more frames than this */
    uint32_t channels;         /* 1 or 2, a layout the module listed */
    uint32_t controls;         /* encoders that can reach on_event(), 0 if none */
    const char *device;        /* family name, e.g. "resona-box"; for logs, not for logic */
    /* Diagnostics for init() only; may be NULL. Never call from process(). */
    void (*log)(const char *message);
    uint32_t struct_size; /* present for modules with memory_profile tail */
    const resona_memory_context *memory; /* valid for the instance lifetime */
    const resona_tools *tools; /* optional; inspect struct_size before reading */
} resona_host;

/* Older memory-aware hosts may omit tools. A null result selects portable DSP. */
static inline const resona_tools *resona_host_tools(const resona_host *h) {
    if(!h || h->struct_size<sizeof(resona_host) || !h->tools ||
       h->tools->version!=RESONA_TOOLS_VERSION || h->tools->struct_size<sizeof(resona_tools))return NULL;
    return h->tools;
}

/* ---- Screen ----------------------------------------------------------
 *
 * A module may draw a UI. The host calls ui_draw() a few times a second (and
 * after every parameter change), outside the audio thread. The module issues
 * drawing calls on the handle; the host records them into a bounded list and
 * executes the list. The device and the desktop simulator run the same list the
 * same way, so a preview shows what the box will show.
 *
 * Canvas: ui->width x ui->height pixels (the Resona Box: RESONA_UI_WIDTH x
 * RESONA_UI_HEIGHT), origin at the top left of the visible glass, RGB565. Per frame the host keeps at most
 * RESONA_UI_MAX_COMMANDS drawing calls and RESONA_UI_TEXT_BYTES bytes of text;
 * further calls are ignored. Text is a built-in 5x7 face (letters, digits and
 * "- . / + # : < > ! = _ ?"; lowercase draws as uppercase, anything else as
 * '?'), advancing 6 * scale pixels per character.
 */
#define RESONA_UI_WIDTH 320u
#define RESONA_UI_HEIGHT 172u
#define RESONA_UI_MAX_COMMANDS 128u
#define RESONA_UI_TEXT_BYTES 512u

/* Pack 8-bit channels into the RGB565 value the drawing calls take. */
#define RESONA_RGB(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

/* ---- Input -----------------------------------------------------------
 *
 * Encoders are logical controls 0 .. host->controls - 1. On the Resona Box they
 * are SELECTOR (the top one, the select encoder) and CUSTOMIZE (the bottom one,
 * the remappable function encoder); other devices map their own. Each reports
 * turns, short clicks and long presses. The host keeps the gestures it needs for
 * its own navigation (on the Resona Box: SELECTOR long press and double press). A module that wants them
 * provides on_event(); the host calls it from the screen thread, outside the
 * audio thread, once per screen refresh for every queued event, in order.
 *
 * Events are gestures, already classified by the box: a click arrives on
 * release, a long press when the hold time is reached. Quick turns are merged
 * into one event with the summed detent count. At most 32 events are queued;
 * further ones are dropped and counted. The module's own UI is the only place a
 * user sees the effect, so update the state that ui_draw() reads.
 *
 * on_event() may run while process() runs: write plain values the way
 * set_param() does and keep it short and bounded. One that runs too long is
 * stopped and the effect is bypassed.
 */
#define RESONA_EVENT_TURN 1u  /* value: signed detents, + is clockwise */
#define RESONA_EVENT_CLICK 2u /* value: 1 */
#define RESONA_EVENT_LONG 3u  /* value: 1 */
#define RESONA_EVENT_DOUBLE 4u /* value: 1; only when the host enables double clicks (box apps) */

#define RESONA_INPUT_SELECTOR 0u  /* encoder 1, top */
#define RESONA_INPUT_CUSTOMIZE 1u /* encoder 2, bottom */

typedef struct resona_event {
    uint32_t kind;  /* RESONA_EVENT_* */
    uint32_t input; /* RESONA_INPUT_* */
    int32_t value;
} resona_event;

/*
 * Drawing primitives, "UI v1". They arrive as extra entries of the handle, present when
 * `ext == RESONA_UI_EXT_MAGIC` (a box on an older firmware has none, and a module should
 * go through the helpers in <resona/ui.h> and the widgets of <resona/widgets.h>, which
 * fall back to text). Every one is one draw call under the same budget as `rect`; all are
 * 1 pixel wide, integer-only and clipped to the canvas, and the device and the desktop
 * produce identical pixels. Coordinates beyond +-2048 and radii beyond 512 are ignored.
 *
 *   line          Bresenham line, both ends included
 *   ellipse       midpoint ellipse outline, semi-axes rx and ry (0 draws a line or a point)
 *   ellipse_fill  the same, filled
 *   dots          `show` of `count` square dots of `size` pixels spaced evenly along an
 *                 elliptical arc: `start` degrees clockwise from 12 o'clock, `sweep` degrees
 *   poly          closed polygon outline, 2 to 8 vertices as x, y pairs
 *   poly_fill     filled convex polygon, 3 to 8 vertices; its outline is included
 *   dotline       a line drawn only at every `step`th pixel (the box's dotted rule)
 */
#define RESONA_UI_EXT_MAGIC 0x31495552u /* "RUI1" */
#define RESONA_UI_MAX_POLY 8u

typedef struct resona_ui {
    uint16_t width;
    uint16_t height;
    void *ctx;
    /* Fill the whole canvas. */
    void (*clear)(void *ctx, uint16_t color);
    /* Filled rectangle; clipped to the canvas. */
    void (*rect)(void *ctx, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
    /* UTF-8 text at (x,y); scale1..8. The small built-in font uses one
     * fallback per unsupported scalar. App/project fonts use resona/font.h. */
    void (*text)(void *ctx, int32_t x, int32_t y, uint32_t scale, uint16_t color,
                 const char *s);
    /* ---- UI v1: valid only when ext == RESONA_UI_EXT_MAGIC ---- */
    uint32_t ext;
    void (*line)(void *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
    void (*ellipse)(void *ctx, int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t color);
    void (*ellipse_fill)(void *ctx, int32_t cx, int32_t cy, int32_t rx, int32_t ry,
                         uint16_t color);
    void (*dots)(void *ctx, int32_t cx, int32_t cy, int32_t rx, int32_t ry, int32_t start_deg,
                 int32_t sweep_deg, uint32_t count, uint32_t show, uint32_t size,
                 uint16_t color);
    void (*poly)(void *ctx, const int16_t *xy, uint32_t n, uint16_t color);
    void (*poly_fill)(void *ctx, const int16_t *xy, uint32_t n, uint16_t color);
    void (*dotline)(void *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t step,
                    uint16_t color);
} resona_ui;

/* Optional workload for desktop stress/check tools. The runtime-only1.02 OS
 * does not execute these hooks or reserve CPU from this profile. Its public
 * layout remains available to host tools. See the SDK runtime-budget guide. */
#define RESONA_CPU_PROFILE_VERSION 1u
#define RESONA_CPU_TRIAL_ACTIVE 0u
#define RESONA_CPU_TRIAL_TRANSITION 1u
#define RESONA_CPU_TRIAL_RELEASE 2u
typedef struct resona_cpu_profile {
    uint32_t version;       /* RESONA_CPU_PROFILE_VERSION */
    uint32_t struct_size;   /* sizeof(resona_cpu_profile) */
    uint32_t min_permille;  /* 0..800 metadata; not used for on-device CPU admission. */
    void (*prepare)(void *state, uint32_t phase); /* optional, no audio/IO here */
} resona_cpu_profile;

typedef struct resona_module {
    uint32_t abi;         /* RESONA_MODULE_ABI */
    uint32_t struct_size; /* sizeof(resona_module) */
    const char *id;       /* reverse-DNS, e.g. "org.example.warm-drive" */
    const char *name;
    const char *version;
    uint32_t layouts;     /* RESONA_LAYOUT_* the module can run in */
    uint32_t rates;       /* RESONA_RATE_* the module can run at */
    uint32_t state_size;  /* bytes the host allocates, RESONA_STATE_ALIGN aligned */
    uint32_t param_count; /* ABI2: bounded by package metadata/image and runtime resources */
    const resona_param *params;
    const resona_param_ext *param_ext; /* NULL, or param_count entries */

    /* Called once with a zeroed state block. Set defaults and coefficients for
     * host->sample_rate and host->channels. */
    void (*init)(void *state, const resona_host *host);
    /* Clear signal history (delay lines, filters) without touching params. */
    void (*reset)(void *state);
    /* `value` is already clamped to the parameter's [min, max]. */
    void (*set_param)(void *state, uint32_t index, float value);
    /* Process `frames` frames (1 .. host->max_block_frames) of host->channels
     * channels: in[c] and out[c] for each channel c. */
    void (*process)(void *state, const float *const *in, float *const *out, uint32_t frames);

    /* Optional: draw the screen. NULL for a module that uses the host's
     * parameter page. Runs outside the audio thread; read the state, never write
     * it. Must be finite and quick: a call that runs too long is stopped and the
     * effect is bypassed. */
    void (*ui_draw)(void *state, const resona_ui *ui);

    /* Optional: an encoder event (see "Input" above). NULL to ignore input. */
    void (*on_event)(void *state, const resona_event *event);

    /* Optional parameter feedback. Return the current value in parameter units,
     * including edits made by on_event(). The host reads all declared parameters
     * after an event and before preset Save, outside the audio thread. Values
     * must be finite; the host clamps/quantizes them and calls set_param() for
     * changes. Read only: never change state or log here. */
    float (*get_param)(const void *state, uint32_t index);
    /* Optional desktop workload tail; ignored for on-device runtime admission. */
    const resona_cpu_profile *cpu_profile;
    /* Optional immutable requests. Buffers are
     * allocated and zeroed before init(), or loading fails without replacement. */
    const resona_memory_profile *memory_profile;
    /* Optional ABI-1 tail: 0/RESONA_MEMORY_FAST (default) or RESONA_MEMORY_PSRAM.
     * State and buffers share live device budgets; no automatic fallback. */
    uint32_t state_region;
    /* Optional immutable code placement. NULL selects the dedicated fast pool.
     * Explicit SRAM/PSRAM has no automatic fallback. */
    const resona_code_profile *code_profile;
} resona_module;

#define RESONA_MODULE_STATE_REGION_SIZE ((uint32_t)offsetof(resona_module, code_profile))

static inline uint32_t resona_module_state_region(const resona_module *m) {
    return m && m->struct_size>=RESONA_MODULE_STATE_REGION_SIZE && m->state_region
        ?m->state_region:RESONA_MEMORY_FAST;
}

static inline uint32_t resona_module_code_region(const resona_module *m) {
    return m && m->struct_size>=sizeof(resona_module) && m->code_profile
        ?m->code_profile->region:RESONA_CODE_FAST;
}

/* ABI-1 prefix retained so previously installed packs keep working. Rebuild a
 * custom-UI pack with get_param to report its own parameter edits. */
#define RESONA_MODULE_MEMORY_PROFILE_SIZE ((uint32_t)offsetof(resona_module, state_region))
#define RESONA_MODULE_BASE_SIZE ((uint32_t)offsetof(resona_module, get_param))
#define RESONA_MODULE_CPU_PROFILE_SIZE ((uint32_t)offsetof(resona_module, memory_profile))
#define RESONA_MODULE_GET_PARAM_SIZE ((uint32_t)offsetof(resona_module, cpu_profile))

/* The single symbol a module must export. */
RESONA_EXPORT const resona_module *resona_module_get(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RESONA_MODULE_H */
