/* GPL-3.0. Presets: 50 stored chains of effects, and the one playing now. */
#ifndef RESONA_PROGRAM_H
#define RESONA_PROGRAM_H
#include <stdint.h>
#define RESONA_PROGRAM_PRESETS 50u
#define RESONA_PROGRAM_NO_PARAMETER UINT32_MAX
#define RESONA_PROGRAM_USED 4u

/* The preset playing now. encoder_effect/encoder_parameter: the parameter mapped
 * to Encoder 2, or RESONA_PROGRAM_NO_PARAMETER for both. */
typedef struct resona_program_info {
 uint32_t version,size,effect_count,enabled,encoder_effect,encoder_parameter,flags;
 int32_t active_preset; /* 0..49, -1 if none */
 char name[17];
 uint8_t reserved[3];
} resona_program_info;

/* Jobs own loading: the platform checks CPU and memory before a chain plays, so
 * callers never block on it. Poll program_job; COMPLETE means committed. */
#define RESONA_PROGRAM_LOAD 1u
#define RESONA_PROGRAM_PREVIEW 2u
#define RESONA_PROGRAM_SAVE 3u
#define RESONA_PROGRAM_RECHECK 4u /* reserved action; runtime-only OS returns ERR_ARG */
#define RESONA_PROGRAM_FACTORY_RESET 5u
#define RESONA_PROGRAM_EDIT 16u /* job action reported for program_edit() */
#define RESONA_PROGRAM_IDLE 0u
#define RESONA_PROGRAM_QUEUED 1u
#define RESONA_PROGRAM_PREPARING 2u
#define RESONA_PROGRAM_CHECKING 3u
#define RESONA_PROGRAM_STARTING 4u
#define RESONA_PROGRAM_COMPLETE 5u
#define RESONA_PROGRAM_FAILED 6u
#define RESONA_PROGRAM_CANCELLED 7u
typedef struct resona_program_job {
 uint32_t version,size,id,action,preset,state,phase,done,total;
 int32_t result;
 /* Legacy CPU-cache slots remain zero; no on-device cache or qualification. */
 uint32_t bank_crc32,elapsed_ms,cache_hits,qualification_misses,cache_entries,cache_bytes;
 int32_t cache_write_error;
 uint32_t queued_id,queued_preset,flags;
 uint32_t active_preset,committed_preset,epoch,owner,cache_writes,boot;
} resona_program_job;

/* The chain. Built-in engines (closed, visible only through their parameters)
 * and installed RPK effects share one shape. Effect and parameter indices
 * address the chain playing now, until the next load or edit. */
#define RESONA_CHAIN_MAX 5u
#define RESONA_CHAIN_BUILTIN 1u
#define RESONA_CHAIN_RPK 2u
#define RESONA_CHAIN_NO_MODEL (-1)
typedef struct resona_chain_effect {
 uint32_t source;          /* RESONA_CHAIN_BUILTIN or RESONA_CHAIN_RPK */
 uint32_t engine;          /* RESONA_ENGINE_* for a built-in, 0 for an RPK */
 uint32_t enabled;
 uint32_t parameter_count;
 int32_t model;            /* built-in model 0..29, or RESONA_CHAIN_NO_MODEL */
 uint32_t variant;         /* built-in variant (Gate: 1 = Auto Gate) */
 uint32_t tripped;         /* bypassed by its time guard */
 char name[24];
} resona_chain_effect;
typedef struct resona_chain_view {
 uint32_t version,size,count;
 int32_t active_preset;    /* 0..49, -1 if none */
 resona_chain_effect effects[RESONA_CHAIN_MAX];
} resona_chain_view;

/* A preset is one of two shapes: the factory chain (Gate, Comp, Resona, NAM, Cab, each
 * once, in any order) or one RPK effect on its own. Edits are jobs, like loads:
 * SET makes the playing preset the factory chain at its defaults (`source` BUILTIN,
 * `engine` 0) or the one RPK `entry` at its defaults (`source` RPK); MOVE moves
 * `effect` to `to` within the factory chain. */
#define RESONA_CHAIN_MOVE 3u
#define RESONA_CHAIN_SET 5u
typedef struct resona_chain_edit {
 uint32_t version,size,op,effect,to;
 uint32_t source;          /* RESONA_CHAIN_BUILTIN or RESONA_CHAIN_RPK */
 uint32_t engine;          /* built-in engine */
 uint32_t entry;           /* RPK: RESONA_ENTRY(RESONA_ENTRY_PACK, pool id) */
} resona_chain_edit;

/* CPU and memory of the chain playing now. Timing is in 4 MHz wall ticks; the
 * heaps are what the platform allocated, not the whole physical RAM. */
typedef struct resona_program_resources {
 uint32_t block_frames, period_ticks, budget_ticks;
 /* reserved_ticks carries rolling DSP mean on runtime-only1.02, not a reservation. */
 uint32_t audio_last_ticks, audio_peak_ticks, reserved_ticks;
 uint32_t fast_total, fast_free, psram_total, psram_free;
 uint32_t code_total, code_used;
} resona_program_resources;
#endif
