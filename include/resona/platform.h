/*
 * Resona platform API, version 1.
 *
 * A box app is the box's application: its UI, operation logic, MIDI/SysEx protocol,
 * Bluetooth control protocol and settings. It is built with Resona Lab, installed on the
 * box, and loaded by the platform at boot. See the SDK's Box App developer guide.
 *
 * The platform keeps what must stay controlled: start-up, recovery and updates, the drivers,
 * the audio thread and the factory engines, the effect runtime and storage, and a reserved
 * management channel no app can take over.
 *
 * Rules
 *   - The app reaches the hardware only through `resona_platform`. It never calls a firmware
 *     address directly, so it keeps working across platform updates of the same API version.
 *   - `version` and `size` must match what the App was built against; the platform refuses
 *     an App built for another version.
 *   - Nothing here runs in the audio thread. Calls that change the sound are validated and
 *     applied by the platform at block boundaries.
 *   - Return values: 0 or a count on success, a negative RESONA_ERR_* on failure.
 *
 * GPL-3.0 licensed, see LICENSE.
 */
#ifndef RESONA_PLATFORM_H
#define RESONA_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#include "resona/module.h" /* resona_ui, resona_event, resona_param_ext */
#include "resona/rgb.h"
#include "resona/startup.h"
#include "resona/bluetooth.h"
#include "resona/effects.h"
#include "resona/app_memory.h"
#include "resona/program.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RESONA_PLATFORM_VERSION 1u

/* Scalar limits; invalid lengths/alignments are recoverable errors. */
#define RESONA_APP_MIDI_EVENT_MAX_BYTES 256u
#define RESONA_APP_MIDI_SEND_MAX_BYTES 512u
#define RESONA_APP_ALLOC_MAX_ALIGN 4096u
#define RESONA_APP_TIMER_MAX_PERIOD_MS 2147483647u
#define RESONA_APP_MAX_TIMERS 8u
#define RESONA_APP_MAX_ALLOCATIONS 64u


/* ---- Errors ---------------------------------------------------------------- */

#define RESONA_OK 0
#define RESONA_ERR_ARG -1         /* a value is out of range */
#define RESONA_ERR_STATE -2       /* not now (busy, audio restarting, nothing there) */
#define RESONA_ERR_NO_MEMORY -3
#define RESONA_ERR_STORAGE -4     /* flash read/write failed */
#define RESONA_ERR_UNSUPPORTED -5 /* this box or platform version lacks it */
#define RESONA_ERR_TIMEOUT -6
#define RESONA_ERR_BUDGET -9      /* resource budget error; DSP overloads are reported at runtime */
#define RESONA_ERR_FULL -8        /* storage full; nothing was saved */
#define RESONA_ERR_DENIED -7      /* reserved for the platform (e.g. a reserved SysEx op) */
#define RESONA_ERR_PACK_ID -10        /* no installed algorithm with this ID */
#define RESONA_ERR_PACK_VERSION -11   /* ID exists, requested version does not */
#define RESONA_ERR_PACK_CRC -12       /* ID/version exists, reviewed CRC differs */
#define RESONA_ERR_PACK_AMBIGUOUS -13 /* more than one package matches */
#define RESONA_ERR_PACK_FORMAT -14    /* invalid/corrupt stored package */
#define RESONA_ERR_PACK_TARGET -15    /* selected package has no compatible code */
/* Preset-job results (resona_program_job.result, program_submit/program_edit). */
#define RESONA_ERR_BUSY -16           /* the preset store is busy; retry */
#define RESONA_ERR_RECORD -17         /* a stored preset record is invalid */
#define RESONA_ERR_CHANGED -18        /* the preset bank changed since it was read; refresh */
#define RESONA_ERR_SELECTION -20      /* saved, but the active preset was not updated */
#define RESONA_ERR_CANCELLED -21      /* replaced by a newer request */
#define RESONA_ERR_TIME_LIMIT -23     /* preparing the preset exceeded its time limit */
#define RESONA_ERR_BOOT_SKIPPED -24   /* the boot restore was skipped; choose a preset */
#define RESONA_ERR_NO_AUDIO -25       /* audio was not ready at boot; the preset was not restored */

static inline const char *resona_error_string(int error)
{
    switch(error){
    case RESONA_OK:return "OK";
    case RESONA_ERR_ARG:return "Invalid argument";
    case RESONA_ERR_STATE:return "Not ready or busy";
    case RESONA_ERR_NO_MEMORY:return "Insufficient memory";
    case RESONA_ERR_STORAGE:return "Storage read/write failure";
    case RESONA_ERR_UNSUPPORTED:return "Unsupported operation";
    case RESONA_ERR_TIMEOUT:return "Operation timed out";
    case RESONA_ERR_DENIED:return "Operation denied";
    case RESONA_ERR_FULL:return "Storage full";
    case RESONA_ERR_BUDGET:return "CPU budget exceeded";
    case RESONA_ERR_PACK_ID:return "Package ID not found";
    case RESONA_ERR_PACK_VERSION:return "Package version mismatch";
    case RESONA_ERR_PACK_CRC:return "Package CRC mismatch";
    case RESONA_ERR_PACK_AMBIGUOUS:return "Ambiguous package selection";
    case RESONA_ERR_PACK_FORMAT:return "Invalid stored package";
    case RESONA_ERR_PACK_TARGET:return "Package target unsupported";
    case RESONA_ERR_BUSY:return "Preset store busy";
    case RESONA_ERR_RECORD:return "Invalid preset record";
    case RESONA_ERR_CHANGED:return "Presets changed; refresh";
    case RESONA_ERR_SELECTION:return "Saved; selection not updated";
    case RESONA_ERR_CANCELLED:return "Cancelled by a newer request";
    case RESONA_ERR_TIME_LIMIT:return "Preset preparation timed out";
    case RESONA_ERR_BOOT_SKIPPED:return "Boot restore skipped";
    case RESONA_ERR_NO_AUDIO:return "Audio not ready at boot";
    default:return "Unknown error";
    }
}

/* Overflow-safe scalar range validation; does not dereference a buffer.
 * Empty spans at capacity are valid, but spans starting beyond it are not. */
static inline int resona_app_store_range_valid(uint32_t capacity, uint32_t offset,
                                               uint32_t bytes)
{
    return offset <= capacity && bytes <= capacity - offset;
}


/* ---- Events the platform delivers to the app ---------------------------------
 *
 * All events reach `resona_app.event` on the app's main thread, in order.
 * The isolated runtime supports events/timers; background threads are not yet supported.
 */
#define RESONA_APP_EV_INPUT 1   /* encoder gesture: .input */
#define RESONA_APP_EV_KEY 2     /* raw press/release of a push or footswitch: .key */
#define RESONA_APP_EV_MIDI 3    /* a MIDI message (complete SysEx, or 1-3 byte message): .midi */
#define RESONA_APP_EV_CTRL 4    /* a wireless control packet: .ctrl */
#define RESONA_APP_EV_CHANGED 5 /* platform state changed by someone else: .changed */
#define RESONA_APP_EV_TIMER 6   /* a timer the app started: .timer */
#define RESONA_APP_EV_AUDIO 7   /* audio restarted (block size, rate) or an effect was bypassed */

/* Where a MIDI message came from / goes to. */
#define RESONA_MIDI_USB 0u
#define RESONA_MIDI_BLE 1u  /* MIDI over Bluetooth LE via the wireless controller */
#define RESONA_MIDI_UART 2u /* the 3.5 mm / DIN MIDI port, where the box has one */

/* What changed (RESONA_APP_EV_CHANGED). `index` is an effect index, preset slot, etc. */
#define RESONA_CHANGED_PRESET 1  /* a preset was recalled by the platform (reserved channel) */
#define RESONA_CHANGED_EFFECT 2  /* an effect's on/off, parameters, model or variant */
#define RESONA_CHANGED_CHAIN 3   /* the preset's effect was set, or the factory chain reordered */
#define RESONA_CHANGED_MODELS 4  /* models or packs were installed or removed (Lab) */
#define RESONA_CHANGED_GLOBAL 5  /* volume, global EQ, input gain, bypass */
#define RESONA_CHANGED_WIRELESS 6
#define RESONA_CHANGED_PROGRAM_JOB 8 /* asynchronous programme preparation completed */
#define RESONA_CHANGED_USB 7     /* USB host connected / disconnected */

typedef struct resona_app_event {
    uint32_t kind; /* RESONA_APP_EV_* */
    uint32_t time_ms;
    union {
        resona_event input; /* RESONA_EVENT_TURN / CLICK / LONG on RESONA_INPUT_* */
        struct {
            uint32_t key;   /* 0 = encoder 1 push, 1 = encoder 2 push, 2.. = footswitches */
            uint32_t down;  /* 1 pressed, 0 released */
        } key;
        struct {
            uint32_t port;  /* RESONA_MIDI_* */
            uint32_t len;
            const uint8_t *data; /* valid during the call only */
        } midi;
        struct {
            uint32_t len;
            const uint8_t *data; /* one wireless control packet payload, valid during the call */
        } ctrl;
        struct {
            uint32_t what;  /* RESONA_CHANGED_* */
            uint32_t index;
        } changed;
        struct {
            uint32_t id;
        } timer;
        struct {
            uint32_t block_frames;
            uint32_t sample_rate;
            int32_t tripped_role; /* a pack bypassed by its time guard, or -1 */
        } audio;
    } u;
} resona_app_event;

/* ---- Built-in effects ---------------------------------------------------------------
 *
 * Engines compiled into the platform. Together they form the factory chain, one of the
 * two preset shapes (the other is one RPK effect; program.h). An App effect graph can also
 * create them (effects.h). They are visible only through their parameters.
 */
#define RESONA_ENGINE_NONE 0u
#define RESONA_ENGINE_GATE 1u    /* variants: 0 gate, 1 auto gate */
#define RESONA_ENGINE_COMP 2u
#define RESONA_ENGINE_AMP 3u     /* Resona amp profile */
#define RESONA_ENGINE_NAM 4u     /* neural amp model (NAM) */
#define RESONA_ENGINE_CAB 5u     /* impulse response */

/* ---- Stored entries: models and packs ------------------------------------------------
 *
 * An entry id is (kind << 8) | index: index is the model slot within its bank (0..29 on the
 * Resona Box) or, for packs, the pool id (1..255).
 */
#define RESONA_ENTRY_AMP 1u
#define RESONA_ENTRY_NAM 2u
#define RESONA_ENTRY_IR 3u
#define RESONA_ENTRY_PACK 4u
#define RESONA_ENTRY(kind, index) (((uint32_t)(kind) << 8) | (uint32_t)(index))
#define RESONA_ENTRY_KIND(entry) ((uint32_t)(entry) >> 8)
#define RESONA_ENTRY_INDEX(entry) ((uint32_t)(entry) & 0xFFu)

typedef struct resona_entry_info {
    uint32_t id;        /* RESONA_ENTRY(kind, index): what the other entry calls take */
    uint32_t kind;      /* RESONA_ENTRY_* */
    uint32_t bytes;
    uint32_t reserved_area; /* 1 in the reserved model area, 0 in the shared pool */
    uint8_t color[3];
    uint8_t pad;
    char name[24];
} resona_entry_info;

typedef struct resona_storage_info {
    uint32_t pool_bytes;
    uint32_t pool_free;
    uint32_t reserved[4]; /* per RESONA_ENTRY_*: reserved slots (index = kind) */
    uint32_t used[5];     /* per RESONA_ENTRY_*: entries stored */
} resona_storage_info;

/* ---- Presets -------------------------------------------------------------------- */

#define RESONA_PRESET_NAME 16u

typedef struct resona_preset_info {
    uint32_t used;
    uint32_t enabled;
    uint32_t effect_count;
    char name[RESONA_PRESET_NAME + 1];
    uint8_t reserved[3];
} resona_preset_info;

/* ---- Settings (platform-owned, persisted by the platform) ---------------------------- */

#define RESONA_SETTING_VOLUME 1       /* master volume 0..100 */
#define RESONA_SETTING_BYPASS 2       /* 0/1: dry path around the whole chain */
#define RESONA_SETTING_INPUT_GAIN 3   /* dB, -20..20 */
#define RESONA_SETTING_LOOPBACK 4     /* 0/1: USB audio loopback */
#define RESONA_SETTING_USB_VOLUME 5   /* 0..100 */
#define RESONA_SETTING_BT_ENABLE 6    /* 0/1 */
#define RESONA_SETTING_BT_VOLUME 7    /* 0..100 */
#define RESONA_SETTING_MIDI_CHANNEL 8 /* 0 = omni, 1..16: the platform's MIDI CC/PC mapping */
#define RESONA_SETTING_BLOCK_FRAMES 9 /* 64, 128, 256; changing it restarts audio */

/* Validate the complete EQ request before applying or saving any part of it. */
static inline int resona_global_eq_valid(uint32_t enabled, const int8_t bands[5],
                                        int8_t level, uint32_t persist)
{
    if (!bands || enabled > 1u || persist > 1u || level < -24 || level > 24) return 0;
    for (unsigned i = 0; i < 5; ++i) if (bands[i] < -30 || bands[i] > 30) return 0;
    return 1;
}

/* Input behaviour (input_config flags), per encoder. */
#define RESONA_INPUT_PRESS_ON_DOWN 0x1u /* a click is reported on press, not on release */
#define RESONA_INPUT_DOUBLE 0x2u        /* report RESONA_EVENT_DOUBLE (delays single clicks) */

/* The Resona Box's status light (led_set). */
#define RESONA_LED_OFF 0u
#define RESONA_LED_HOVER 1u
#define RESONA_LED_EDIT 2u
#define RESONA_LED_PULSE 3u      /* short flash: a gesture was taken */
#define RESONA_LED_PULSE_LONG 4u
#define RESONA_LED_BYPASS 5u

/* ---- Device -------------------------------------------------------------------- */

typedef struct resona_device_info {
    char product[24];        /* "Resona Box" */
    char family[16];         /* "resona-box" */
    char platform_version[16];
    char serial[33];         /* opaque unit serial (may be empty) */
    uint32_t sample_rate;
    uint32_t block_frames;   /* current */
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t encoders;
    uint32_t footswitches;
    uint32_t safe_mode;      /* 1: the platform's safe UI is running instead of this app */
    uint32_t developer;      /* reserved */
} resona_device_info;


/* ---- The platform function table ------------------------------------------------- */

typedef struct resona_platform {
    uint32_t version; /* RESONA_PLATFORM_VERSION of this platform */
    uint32_t size;    /* sizeof(resona_platform) of this platform */

    /* System */
    void (*device_info)(resona_device_info *out);
    uint32_t (*millis)(void);
    void (*log)(const char *line);      /* to the debug console and Lab's log */
    void (*reboot)(void);
    int (*reserved_0)(void);            /* reserved; returns RESONA_ERR_UNSUPPORTED */
    void (*sleep_ms)(uint32_t ms);      /* at most 50 ms; the App shares the UI thread */
    int (*timer_start)(uint32_t id, uint32_t period_ms, uint32_t repeat);
    void (*timer_stop)(uint32_t id);
    void *(*alloc)(uint32_t bytes, uint32_t align);
    void (*free)(void *p);
    void (*post)(const resona_app_event *ev);
    int (*format)(char *out, uint32_t cap, const char *fmt, ...);
    /* Memory banks: reserve a budget, then allocate from it (app_memory.h). */
    int (*memory_reserve)(uint32_t bank, uint32_t budget_bytes);
    void *(*memory_alloc)(uint32_t bank, uint32_t bytes, uint32_t align);
    int (*memory_info)(uint32_t bank, resona_app_memory_info *out);

    /* Screen, input and lights */
    resona_ui *(*screen_begin)(uint16_t **pixels, uint32_t *stride_px); /* the back buffer */
    void (*screen_present)(int32_t x, int32_t y, int32_t w, int32_t h);
    void (*screen_brightness)(uint32_t percent);
    void (*input_config)(uint32_t input, uint32_t flags); /* RESONA_INPUT_* flags */
    void (*led_set)(uint32_t mode);                       /* RESONA_LED_* */
    int (*rgb_set)(const resona_rgb_config *config);
    int (*rgb_get)(resona_rgb_state *state);

    /* Settings and audio */
    int (*setting_get)(uint32_t id, int32_t *value);
    int (*setting_set)(uint32_t id, int32_t value);
    int (*global_eq_set)(uint32_t enabled, const int8_t half_db[5], int8_t level_half_db,
                         uint32_t persist);
    int (*global_eq_get)(uint32_t *enabled, int8_t half_db[5], int8_t *level_half_db);
    void (*meters)(uint8_t *in_level, uint8_t *out_level, uint8_t *cpu_percent); /* levels 0..9 */
    int (*midi_send)(uint32_t port, const uint8_t *data, uint32_t len);
    int (*ctrl_send)(const uint8_t *packet, uint32_t len);

    /* Storage: the App's own store, and the models and effects on the Box */
    uint32_t (*app_store_size)(void);
    int (*app_store_read)(uint32_t offset, void *dst, uint32_t len);
    int (*app_store_write)(uint32_t offset, const void *src, uint32_t len); /* power-safe */
    int (*entry_count)(void);
    int (*entry_info)(uint32_t position, resona_entry_info *out);
    int (*entry_rename)(uint32_t entry, const char *name);
    int (*entry_color)(uint32_t entry, const uint8_t rgb[3]);
    int (*entry_delete)(uint32_t entry);
    void (*storage_info)(resona_storage_info *out);

    /* Startup and Bluetooth */
    int (*startup_info)(resona_startup_info *out);
    int (*startup_usb_set)(const resona_usb_names *names);
    int (*startup_splash_set)(const void *data, uint32_t bytes);
    int (*bluetooth_names_get)(resona_bluetooth_info *out);
    int (*bluetooth_names_set)(const resona_bluetooth_names *names);

    /* Presets (program.h): 50 stored chains and the one playing now. Loads, saves and
     * chain edits are jobs: submit, then poll program_job (RESONA_CHANGED_PROGRAM_JOB). */
    int (*preset_info)(uint32_t slot, resona_preset_info *out);
    int (*program_info)(resona_program_info *out);
    int (*program_submit)(uint32_t action, uint32_t preset, const char *name, uint32_t expected_crc32);
    int (*program_edit)(const resona_chain_edit *edit);      /* returns a job id */
    int (*program_job)(resona_program_job *out);
    int (*program_enable)(uint32_t enabled);
    int (*program_map_encoder)(uint32_t effect, uint32_t index); /* NO_PARAMETER clears */
    void (*program_resources)(resona_program_resources *out);
    int (*program_chain)(resona_chain_view *out);
    int (*program_effect_param_info)(uint32_t effect, uint32_t index, resona_effect_param *out);
    int (*program_effect_param_get)(uint32_t effect, uint32_t index, float *out);
    int (*program_effect_param_set)(uint32_t effect, uint32_t index, float value);
    int (*program_effect_enable)(uint32_t effect, uint32_t enabled);
    int (*program_effect_model)(uint32_t effect, uint32_t model);     /* built-ins with models */
    int (*program_effect_variant)(uint32_t effect, uint32_t variant); /* Gate: 1 = Auto Gate */
    /* An RPK effect with its own screen draws into the App's back buffer. */
    int (*program_effect_screen_draw)(uint32_t effect, uint32_t force);
    int (*program_effect_screen_event)(uint32_t effect, const resona_event *ev);

    /* App-owned audio (effects.h): an App may build its own effect graph instead of
     * playing presets. While it owns the graph, preset loads and edits are refused. */
    int (*effects_begin)(void);
    int (*effect_create)(const resona_effect_source *source, resona_effect_handle *out);
    int (*effect_destroy)(resona_effect_handle handle);
    int (*effect_param_set)(resona_effect_handle handle, uint32_t index, float value);
    int (*effect_enable)(resona_effect_handle handle, uint32_t enabled);
    int (*effects_graph_set)(const resona_effect_graph *graph);
    int (*effect_info)(resona_effect_handle handle, resona_effect_info *out);
    int (*effect_param_info)(resona_effect_handle handle, uint32_t index, resona_effect_param *out);
    int (*effects_status)(resona_effects_status *out);
} resona_platform;

/* ---- What an app exports ----------------------------------------------------------- */

typedef struct resona_app {
    uint32_t platform_version; /* the RESONA_PLATFORM_VERSION it was built against */
    uint32_t struct_size;      /* sizeof(resona_app) */
    const char *id;            /* reverse-DNS */
    const char *name;
    const char *version;
    uint32_t state_size;       /* bytes the platform allocates (zeroed) for `state` */
    uint32_t heap_bytes;       /* most it will ask from platform->alloc */

    /* Once, on the app's main thread, before anything is drawn. Return 0, or the box starts
     * in safe mode. */
    int (*init)(void *state, const resona_platform *platform);
    /* Every event, in order (input, MIDI, wireless controller packets, changes, timers). */
    void (*event)(void *state, const resona_app_event *ev);
    /* About 30 times a second: animate and draw. `ms` is the time since the last call. */
    void (*frame)(void *state, uint32_t ms);
} resona_app;

/* The single symbol an app must export. */
RESONA_EXPORT const resona_app *resona_app_get(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RESONA_PLATFORM_H */
