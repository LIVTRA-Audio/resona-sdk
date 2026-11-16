# Writing a box app

A box app is the Resona Box's application: what the screen shows, what the encoders do, how
presets are browsed, which MIDI messages mean what, the settings pages. The platform (the
closed firmware under it) keeps start-up and recovery, the audio thread and the factory
engines, the effect runtime, storage, and the management channel. The app talks to the platform
only through `resona_platform` in `sdk/include/resona/platform.h`; read that header for the
exact signatures. This guide says how the parts fit together.

Start from a template: `app-factory` is the box's real UI (pages, presets, settings, Global
EQ, Encoder 2 mapping) and the best reference for how the API is used; `app-blank` is the
smallest working app; `app-synth` drives an algorithm pack as an instrument.

## Creator Lab workflow

Use New Project → Box App, choose a template, and edit its C/C++ files in the normal
explorer. Build checks the RV32 package and starts the full interactive device UI preview.
Both encoders, timers, settings and temporary local presets are available. Install builds
and reviews a package, then stores the App (at most 512 KB) on the connected Resona Box and
restarts it. No CLI or OS/bootloader flashing is required; existing presets/models survive.

Preview is a separate host process: crashes/timeouts preserve the editor. Its storage is
local and temporary, its asset library starts empty, and it does not simulate DSP,
model/pack loading, pack screens, transport or threads. Synth Box therefore shows its
missing-pack state locally; test instrument playback on a box with Tiny Synth installed.
The worker has normal host permissions; it is not a security sandbox. Restart/Build resets
local simulation state. Use the real box for hardware/CPU/audio checks.

## How an app runs

```c
static const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    "org.example.my-app", "My App", "0.1.0",
    sizeof(State),   /* the platform allocates this, zeroed (at most 64 KiB) */
    64 * 1024,       /* the most you will ask from P->alloc (at most 512 KiB) */
    init, event, frame,
};
const resona_app *resona_app_get(void) { return &kApp; }   /* extern "C" in C++ */
```

- At boot the platform loads the app into memory, runs C++ static constructors, then
  `init(state, platform)`. Keep the `platform` pointer (`P` below). Return 0, or the box starts
  in safe mode.
- `event(state, ev)` gets every event in order: input gestures, MIDI, changes made by
  someone else, timers, audio restarts.
- `frame(state, ms)` runs about 30 times a second: animate and redraw what changed.
- All three run on the app's **main thread**. They must return quickly: no waiting, no long
  loops. Each call may use 200 ms of CPU (`init` 2 s, for building fonts and tables); past
  that the App is stopped and the Box shows the recovery screen. Waiting inside a platform
  call such as `effects_begin` does not count. Use `timer_start` for bounded periodic work (at most8 timers; all32-bit IDs
  are valid). Periods1..2147483647ms; invalid periods return ERR_ARG without
  replacing an existing timer. Exhaustion returns ERR_NO_MEMORY; repeat0 means forever.
  Background threads and RTOS mutexes are unsupported in the protected runtime.
- Apps may use mutable globals and C++ constructors (unlike packs). No exceptions or RTTI.
- The Box ends an App that passes an out-of-range argument to `sleep_ms` (over 50),
  `screen_brightness` (over 100), `input_config` (input over 1, unknown flags), `led_set`
  (over `RESONA_LED_BYPASS`) or `ctrl_send` (over 64 bytes), or a pointer outside its memory.
  Lab's preview stops the App the same way and says why in its log.
- Preset jobs report the codes in `platform.h`, including `RESONA_ERR_FULL`,
  `RESONA_ERR_BUSY`, `RESONA_ERR_CHANGED` and `RESONA_ERR_CANCELLED`;
  `resona_error_string()` names them.
- No C library beyond `resona/libc.h` (memcpy … snprintf). Memory comes from `P->alloc`: power-of-two alignment1..4096, at most64 live allocations on the device; NULL on invalid requests or exhaustion. The byte budget and alignment padding also constrain fit.
- Limits on the Resona Box: App size 512 KB (program and variables, and the App file);
  `state_size` 64 KB (allocate more with `P->alloc`), heap 512 KB, event queue 32
  events (more are dropped), MIDI/SysEx messages up to 256 bytes per event.
  `post()` copies the event and valid MIDI/CTRL payload before returning. Payload
  pointers received by `event()` are valid only during that callback. The queue
  includes an event being dispatched and is shared with platform notifications;
  use an App acknowledgement protocol when reliable delivery matters.

## Safety

- An app that crashes or fails `init` while starting is marked failed; the next boot shows the
  platform's **safe mode** (it says why). Holding **Encoder 1 at power-on** also gives safe
  mode for that boot. Installing a new app always works from safe mode.
- `resona-lab app info` shows what runs and why; `resona-lab app reboot --safe` gives one
  safe-mode boot.

## Screen

320 × 172 pixels, RGB565 (`RESONA_RGB(r, g, b)`).

```c
uint16_t *px; uint32_t stride;
resona_ui *ui = P->screen_begin(&px, &stride);     /* the back buffer */
ui->clear(ui->ctx, RESONA_RGB(10, 12, 18));
ui->text(ui->ctx, 12, 10, 3, RESONA_RGB(255, 160, 0), "HELLO");
px[y * stride + x] = colour;                      /* direct pixels are fine too */
P->screen_present(0, 0, 0, 0);                    /* 0,0,0,0 = the whole screen */
```

`resona_ui` has clear, rect, text (5×7 font, scale 1–8), line, dotline, ellipse,
ellipse_fill, dots, poly, poly_fill (`resona/ui.h` wraps them). Present only the rectangle
you changed when you redraw often (meters): it is cheaper. An RPK effect with its own
screen draws it with `program_effect_screen_draw(effect, force)`; pass encoder events to it with
`program_effect_screen_event`. For richer screens see [UI libraries](ui-libraries.md).

### App-owned fonts and UTF-8

Built-in text is a small5×7 face, with one fallback per unsupported Unicode
scalar. Put your own font assets directly in the App project and draw them
with `<resona/font.h>` / `resona_ui_text_font`. `tools/font.py` converts selected
glyphs from a licensed TTF/OTF into a C header, keeping its notice beside it.
No source TTF, Python or font engine is needed on the Box. Both Apps and block
screens can use the helper; block draw budgets still apply. See the standalone
App-owned Unicode font example. The OS has no LVGL; Apps may supply their own
rendering/layout integration using the framebuffer.

For richer screens, the SDK has two optional libraries an App compiles in: the Box UI widget
kit (`lib/widgets`, about 4 KB) and an LVGL 9 port (`lib/lvgl_port`, about 150 KB). See
[ui-libraries.md](ui-libraries.md).

## Input

Two push-encoders: `RESONA_INPUT_SELECTOR` (Encoder 1, top) and `RESONA_INPUT_CUSTOMIZE`
(Encoder 2, bottom). `RESONA_APP_EV_INPUT` carries a gesture in `ev->u.input`:

| `kind` | meaning |
| --- | --- |
| `RESONA_EVENT_TURN` | `value` = signed detents (+ clockwise) |
| `RESONA_EVENT_CLICK` | a short press (on release, unless press-on-down) |
| `RESONA_EVENT_DOUBLE` | a double press, only if enabled for that encoder |
| `RESONA_EVENT_LONG` | held about 0.6 s |

`P->input_config(input, flags)`: `RESONA_INPUT_DOUBLE` reports double presses (single clicks
then wait about 250 ms); `RESONA_INPUT_PRESS_ON_DOWN` reports a click on press. Raw
press/release arrives as `RESONA_APP_EV_KEY`. The status light: `P->led_set(RESONA_LED_*)` for legacy status modes.

Full RGB is available through `P->rgb_set(&config)` and `P->rgb_get(&state)`:

```cpp
resona_rgb_config c = resona_rgb_solid(255, 30, 80, 128);
c.animation = RESONA_RGB_BREATHE;
c.period_ms = 1800;
P->rgb_set(&c);
```

Check version>=5 and size through rgb_get before reading the tail. Brightness
and colors use linear0..255 intensity. Solid uses period0; other animations
use20..60000ms. Blink/fade use secondary RGB; rainbow ignores base colors; fade
holds the secondary color after one period. NULL releases custom control.
Animation scheduling and PWM are trusted platform services, outside audio.
Do not call these APIs in algorithm process(). A Lab override returns ERR_STATE
to app writes until released; fault recovery restores safe legacy control.
See the RGB Studio example for encoder/MIDI control and custom frame sequences.

## The sound: the factory chain or one RPK

A preset is one of two shapes (`resona/program.h`):

- the **factory chain**: the five built-ins Gate, Comp, Resona (`RESONA_ENGINE_AMP`), NAM and
  Cab, each once, in any order. They are closed, visible only through their parameters; or
- **one RPK effect** on its own, from the effect files stored on the Box.

Nothing else: there are no mixed or longer chains, so an RPK never shares the CPU with the
built-ins.

- `program_chain(&view)`: per effect its source, name, on/off, parameter count, model and
  `tripped` (bypassed by its time guard).
- `program_effect_param_info/get/set(effect, index, ...)` in the effect's own units,
  `program_effect_enable` (an RPK preset: that effect, not the preset; `program_enable`
  switches the preset), `program_effect_model(effect, model)` (Resona/NAM/Cab: the model
  number 0..29, `RESONA_ENTRY_INDEX` of the model's entry from `entry_count`/`entry_info`;
  `RESONA_CHAIN_NO_MODEL` until one is chosen), `program_effect_variant` (Gate: 0 gate,
  1 auto gate; changing it clears an Encoder 2 mapping on the Gate).
- `program_effect_screen_draw/event`: an RPK effect that draws its own editor.
- `program_edit(&edit)`: `RESONA_CHAIN_SET` makes the playing preset the factory chain
  (`source` BUILTIN, `engine` 0) or one RPK (`source` RPK, `entry`), at default values;
  `RESONA_CHAIN_MOVE` reorders the factory chain. An edit is a job (poll `program_job`;
  `RESONA_CHANGED_PROGRAM_JOB` tells you when) because the new sound is checked for CPU and
  memory before it plays. Only a bad version, size or op is refused at once; anything else
  the edit cannot do (MOVE on an RPK preset, an unknown entry, a built-in `engine` other
  than 0) ends the job `RESONA_PROGRAM_FAILED` with `RESONA_ERR_ARG`. The job's action is
  `RESONA_PROGRAM_EDIT`. A refusal leaves the playing preset as it was.
- `program_map_encoder(effect, index)`: the parameter Encoder 2 turns; it follows its effect
  when the factory chain is reordered.

Turning a built-in on is admission and can be refused. Edits change the playing preset;
saving is a separate job. An App that wants its own combination of effects builds an App
effect graph instead (below): it owns audio while it runs.

## Presets

50 stored chains (`RESONA_PROGRAM_PRESETS`). `preset_info(slot, &info)` names a slot;
`program_info(&info)` describes what plays now. Loading, browsing and saving are jobs:
`program_submit(RESONA_PROGRAM_LOAD | PREVIEW | SAVE, slot, name, expected_crc32)`, then
`program_job`. PREVIEW plays a slot without making it active (browsing); SAVE captures the
running values. `program_enable(0/1)` switches the whole chain off and on. At boot the
platform restores the active preset before your `init` runs; read it, don't reload it.

## Settings and global audio

`setting_get/set(RESONA_SETTING_*, value)`: VOLUME 0–100, BYPASS, INPUT_GAIN −20…20 dB,
LOOPBACK, USB_VOLUME, BT_ENABLE, BT_VOLUME, MIDI_CHANNEL (0 = omni), BLOCK_FRAMES 64/128/256
(restarts audio; you get `RESONA_APP_EV_AUDIO` when it is back). Global EQ:
`global_eq_get/set` (five bands 100 Hz–4 kHz and a level, in half dB; `persist` 0 while a
control turns, 1 when it is let go). Both enabled and persist must be0or1;
invalid complete requests return ERR_ARG before changing any EQ field. With
persist0, enable changes as well as bands/level are temporary. `meters(&in, &out, &cpu)`: levels 0–9.

## MIDI and SysEx

MIDI reaches the app as `RESONA_APP_EV_MIDI` (`port` USB, BLE or UART; complete
SysEx frames). Messages over256 bytes are dropped whole. Send1..512 bytes with
`P->midi_send(port, data, len)`; invalid scalar length/port returns ERR_ARG without
faulting the App. Inaccessible user buffers remain protected memory faults. SysEx frames that start
`F0 7D 4E 43` are reserved for the platform and never reach the app; anything else is
yours to define.

## Changes made by someone else

`RESONA_APP_EV_CHANGED` with `what` = `RESONA_CHANGED_PRESET` (a preset was recalled, by Lab
or a host), `EFFECT` (on/off, parameters, model or variant), `CHAIN` (the preset's effect was
set, or the factory chain reordered), `PROGRAM_JOB`, `MODELS` (models or effects installed/removed), `GLOBAL`, `WIRELESS`,
`USB`. Re-read what you show and redraw. `RESONA_APP_EV_AUDIO`: audio restarted
(`block_frames`, `sample_rate`) or an effect was bypassed by its time guard.

## Storage

`app_store_read/write(offset, buf, len)`: the app's own bytes (`app_store_size()`, 4 KiB on
the Resona Box), power-safe, kept across reboots and updates of the same app id.
An invalid offset/length, including unsigned overflow, returns `RESONA_ERR_ARG`
and leaves destination/storage unchanged. Empty spans at offset==size are valid
no-ops with a valid non-NULL buffer. The protected device still faults an App
that supplies inaccessible memory for a valid nonempty span. Use
`resona_app_store_range_valid(capacity,offset,length)` for optional caller checks.
This error-return behavior requires the matching storage-range firmware fix;
SDK headers alone cannot update an older device. Put a magic
number and version at the start and validate on read. Presets, models and settings are
stored by the platform; do not copy them here.

## Not available yet

`ctrl_send` (custom wireless controller packets) returns `RESONA_ERR_UNSUPPORTED`; renaming, recolouring and
deleting models works for some entry kinds only (check the return value).

## Resources

`program_resources(&info)` gives live audio ticks (4 MHz), fast and PSRAM heap capacity/free
bytes and instruction-memory use. The platform measures and admits effects before they play;
treat a refusal as a real resource limit. Built-in memory is reserved by the platform; effect
state never silently falls back to slower RAM. CPU trials are measurements, not a hard
execution-time guarantee; the protected runtime separately contains access faults and stuck
callbacks. Refresh an older project's pinned SDK with `resona-lab sdk sync`.

## Code-first startup customization

Declare USB product/audio/MIDI names in your Box App code using
`resona/startup_helpers.h` and `resona_startup_usb_apply(platform,&names)`.
Negative returns refuse the change;1 means saved/reboot required;0 means active.
Only call reboot when required, so initialization cannot loop resets. There is
no USB naming window or user-supplied VID/PID. The trusted platform manages
profiles; Creator Lab reconnects through reserved CDC, independent of MIDI names.
Names are 1..47 UTF-8 bytes without control characters.

`startup_splash_set(data,bytes)` installs a validated independent RSP1 animation;
NULL/0 restores the factory splash. Duration0 skips; other durations are chosen
by the author. Use the SDK tools/splash.py tool and [startup](startup.md), or generate
the public format from code. Long splashes delay Box App initialization and preset
recall; USB management remains available. Use resona/splash.h to build frames and resona_startup_splash_apply to stage only changed assets; there is no startup setup window.
Do not put any startup operation in an audio callback. Do not repeatedly write
settings every frame. All startup setters are bounded protected control calls.

The platform has `bluetooth_names_get/set` and `<resona/bluetooth_helpers.h>`. Name changes persist in the wireless controller and require disconnecting all power. No radio address, bond or sample-rate control is exposed.


### Resource-admitted effect graphs

Use `<resona/effects.hpp>` and one `resona::Effects` controller for an App-owned
graph. `begin(platform)` transfers audio ownership (the preset chain is suspended); `factory(engine)` and
`pack(id,version,crc32)` return independent handles. `connect(source,destination,gain,
source_channel,destination_channel)`, `output(handle)` and `capture(handle)` build
routes; `commit()` validates structure/memory and copies them. `param`, `enable`, `info`,
`param_info` and `status` operate on handles. Check every returned error/zero handle.
Use `clear_routes()` to build a replacement graph. RAM/code constrain instance
creation; CPU overload is handled at runtime and reported to the USB console. USB/Bluetooth sources are stereo,
effects are mono, and summing is explicit. A failed commit retains the old graph.
See [contracts](contracts.md#app-owned-effect-graphs) for lifetime and preview limits.

No import, model-change or graph-commit CPU benchmark runs on the matching1.02
runtime. Actual DSP uses a256-block rolling budget with brief peaks allowed
within physical headroom. Creator Lab reports usage against a100%budget limit.
See [runtime budget](cpu-qualification.md) for fault and reload behavior.

Preview and native built-in metadata use the same public descriptor
table, including Auto Gate SENSITIVITY. `effect_param_info.def` stays constant
after control changes. Failed pack creation distinguishes missing ID, version
mismatch, CRC mismatch, ambiguity, malformed stored data and I/O; see the
ERR_PACK_* constants in platform.h. `resona_error_string(rc)` gives a display
label.
