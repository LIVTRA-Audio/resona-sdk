# Reference

The API limits, versions, headers and error codes in one place.

## Apps

| | |
| --- | --- |
| App size | at most 512 KB |
| `state_size` | at most 64 KB; allocate larger memory with `P->alloc` |
| Heap (`heap_bytes`) | at most 512 KB; 64 live allocations, alignment up to 4096 |
| Own storage | 4 KB (`app_store_read/write`) |
| Time per call | return quickly; the Box stops an App whose call runs too long |
| Frames | about 30 per second |
| Timers | 8; periods 1 ms to about 24.9 days |
| Event queue | 32 events; more are dropped |
| MIDI | messages in up to 256 bytes; out 1 to 512 bytes |

## Effects

| | |
| --- | --- |
| Audio | mono in, mono out; rate and block size come from the host |
| Parameter names | up to 12 characters |
| Own screen | drawn about 20 times a second; up to 128 drawing calls and 512 bytes of text per frame |
| Memory | `State` plus up to 16 named buffers, in fast memory or PSRAM |
| CPU | measured by the Box on first load; it refuses an effect that does not fit |
| CPU profile minimum | 0 to 800 ‰ (0 means measure only) |

## Versions

| | |
| --- | --- |
| SDK | 1.0.0 (`sdk.json`) |
| App platform API | 1 (`RESONA_PLATFORM_VERSION`) |
| Effect module ABI | 2 (`RESONA_MODULE_ABI`) |
| Package format | 1, target `RBX1` |

The Box refuses a package built for a different target; rebuild old `.rpk` files with Lab.

## Headers

| Header | For |
| --- | --- |
| `module.h` | effects: `resona_module`, parameters, screen and event hooks |
| `platform.h` | Apps: `resona_app`, `resona_platform`, settings, errors |
| `program.h` | presets: the chain, live edits, jobs |
| `effects.h`, `effects.hpp` | App effect graphs |
| `blocks.h`, `blocks/` | DSP building blocks |
| `dsp.h`, `math.h` | DSP helpers and maths that match on computer and Box |
| `memory.h`, `app_memory.h`, `code.h` | memory and code placement |
| `parameters.h` | passing knob changes safely to `process()` |
| `ui.h`, `widgets.h`, `font.h`, `utf8.h` | drawing and text |
| `rgb.h` | the status light |
| `startup.h`, `startup_helpers.h`, `splash.h` | USB names and start-up animation |
| `bluetooth.h`, `bluetooth_helpers.h` | Bluetooth names |
| `factory_params.h` | the built-in effects' parameters |
| `pack.h`, `image.h` | the `.rpk` package and device image formats (Lab builds these) |
| `libc.h` | the small C library Apps can use |

## Errors

Calls return 0 on success or one of these; `resona_error_string(rc)` names them.

| Code | Name | Meaning |
| --- | --- | --- |
| -1 | `RESONA_ERR_ARG` | a value is out of range |
| -2 | `RESONA_ERR_STATE` | not now (busy, audio restarting, nothing there) |
| -3 | `RESONA_ERR_NO_MEMORY` | not enough memory |
| -4 | `RESONA_ERR_STORAGE` | storage read or write failed |
| -5 | `RESONA_ERR_UNSUPPORTED` | this Box or API version does not have it |
| -6 | `RESONA_ERR_TIMEOUT` | timed out |
| -7 | `RESONA_ERR_DENIED` | reserved for the Box |
| -8 | `RESONA_ERR_FULL` | storage full; nothing was saved |
| -9 | `RESONA_ERR_BUDGET` | not enough CPU |
| -10 | `RESONA_ERR_PACK_ID` | no stored effect with this ID |
| -11 | `RESONA_ERR_PACK_VERSION` | that version is not stored |
| -12 | `RESONA_ERR_PACK_CRC` | the stored file is a different build |
| -13 | `RESONA_ERR_PACK_AMBIGUOUS` | more than one stored effect matches |
| -14 | `RESONA_ERR_PACK_FORMAT` | the stored effect is invalid |
| -15 | `RESONA_ERR_PACK_TARGET` | the effect was built for a different target |
| -16 | `RESONA_ERR_BUSY` | the preset store is busy; try again |
| -17 | `RESONA_ERR_RECORD` | a stored preset is invalid |
| -18 | `RESONA_ERR_CHANGED` | presets changed since you read them; read again |
| -20 | `RESONA_ERR_SELECTION` | saved, but the playing preset was not updated |
| -21 | `RESONA_ERR_CANCELLED` | replaced by a newer request |
| -23 | `RESONA_ERR_TIME_LIMIT` | preparing the preset took too long |
| -24 | `RESONA_ERR_BOOT_SKIPPED` | the preset was not restored at start; choose one |
| -25 | `RESONA_ERR_NO_AUDIO` | audio was not ready at start; the preset was not restored |

## Simulator and Box

| | Simulator | Box |
| --- | --- | --- |
| Your App's screen, encoders, timers, settings | yes | yes |
| Your effect's parameters and own screen | yes | yes |
| Sound of your effect | **Flow → Listen** | yes |
| Built-in effects (Gate, Comp, Resona, NAM, Cab) | no sound | yes |
| CPU and memory measurement | no | yes |
| Presets and App storage | temporary | kept |
