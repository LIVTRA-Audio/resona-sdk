# Apps

An App is the Box's software: what the screen shows, what the two encoders do, how presets
are browsed and edited, the settings pages and the MIDI it answers. The Box runs one App at
a time. Its factory software is an App too, and its source is in `apps/resona_factory`, so
you can change it or start your own from it.

An App talks to the Box only through `resona_platform` (`include/resona/platform.h`).

## How an App runs

```c
static const resona_app kApp = {
    RESONA_PLATFORM_VERSION, sizeof(resona_app),
    RESONA_APP_ID, RESONA_APP_NAME, RESONA_APP_VERSION,  /* from resona.project.json */
    sizeof(State),   /* your App's memory, zeroed at start */
    64 * 1024,       /* the most you will ask from P->alloc */
    init, event, frame,
};
const resona_app *resona_app_get(void) { return &kApp; }  /* extern "C" in C++ */
```

- `init(state, platform)` runs once at start. Keep the `platform` pointer (`P`). Return 0,
  or the Box starts in safe mode.
- `event(state, ev)` gets everything that happens, in order: encoder turns and clicks,
  MIDI, timers, and changes made by someone else (a preset recalled from Lab, say).
- `frame(state, ms)` runs about 30 times a second: animate and redraw what changed.

All three must return quickly: about 200 ms of CPU per call (2 s for `init`). Use
`P->timer_start` for regular work. Global variables and C++ constructors are fine in an App.

## Screen and controls

- The screen is RGB565; read its size from `P->device_info`. `P->screen_begin` gives you the back buffer and a
  set of drawing calls (text, lines, rectangles, circles, polygons); `P->screen_present`
  shows it. For richer screens use `lib/widgets` or the LVGL port ([UI libraries](../agent/docs/ui-libraries.md)).
- Two push-encoders: **SELECTOR** (Encoder 1, top) and **CUSTOMIZE** (Encoder 2, bottom).
  You get turns, clicks, long presses (about 0.6 s) and, if you turn them on, double presses.
- The status light: `P->rgb_set` for colours, brightness and animations.

## The sound

The Box plays one preset at a time. A preset is either the **factory chain** (Gate, Comp,
Resona, NAM, Cab, each once, in any order) or **one effect**. Your App reads and changes it
with `program.h`:

- `program_chain`: the effects in the playing preset, their names, on/off and models.
- `program_effect_param_info/get/set`, `program_effect_enable`, `program_effect_model`:
  change the playing preset live.
- `program_edit`: make the preset the factory chain or one stored effect
  (`RESONA_CHAIN_SET`), or reorder the factory chain (`RESONA_CHAIN_MOVE`). An edit is a
  job: the Box checks CPU and memory before the new sound plays, and if it does not fit the
  old sound keeps playing.
- `program_submit`: load, save or reset a preset (also a job).
- `program_map_encoder`: which parameter Encoder 2 turns.

An App that wants its own combination of effects builds an **effect graph** with
`<resona/effects.hpp>` (see `apps/parallel_effects`). While it does, the App owns the audio
and the preset is paused.

## Settings, MIDI and storage

- `P->setting_get/set`: volume, bypass, input gain, USB and Bluetooth audio, MIDI channel,
  block size. `P->global_eq_set/get`: the five-band Global EQ.
- MIDI from USB, Bluetooth and the MIDI jack arrives as events; send with `P->midi_send`.
  SysEx that starts `F0 7D 4E 43` belongs to the Box and never reaches your App.
- `P->app_store_read/write`: 4 KB of your App's own storage that survives power cycles.
- `P->entry_count/entry_info`: the effects and models stored on the Box.

## Start-up

Your App can set the Box's USB names, its Bluetooth names and its start-up animation, in
code: see `apps/usb_names`, `apps/bluetooth_names` and `apps/splash_code`. The helpers in
`<resona/startup_helpers.h>` and `<resona/bluetooth_helpers.h>` write only a change and tell
you when a restart is needed. Details: [start-up](../agent/docs/startup.md).

## If something goes wrong

- An App that crashes, or fails `init`, is stopped and the Box shows **safe mode** with the
  reason. You can always install another App from safe mode.
- Holding **Encoder 1 while powering on** starts safe mode for that boot.
- An App that passes an out-of-range value to some calls is stopped too; the Simulator
  stops it the same way and says why in Lab's log.

## Try it, then install

| Step | In Lab | Terminal |
| --- | --- | --- |
| Build and run on your computer | **Build**, then the Simulator | `resona-lab app build`, `resona-lab app preview` |
| Put it on the Box | **Install on Box** | `resona-lab app install-checked` |

The Simulator runs your App's code with the screen, both encoders, settings and temporary
presets. It does not play the Box's built-in effects or measure CPU: check sound and CPU
on the Box.

More: [the detailed App guide](../agent/docs/box-apps.md), [UI libraries](../agent/docs/ui-libraries.md)
and [recipes](../agent/docs/recipes.md).
