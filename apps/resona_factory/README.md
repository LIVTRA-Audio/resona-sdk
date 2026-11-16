# Resona (factory app)

The Resona Box's own UI and operation logic, as a box app on the platform API
([`resona/platform.h`](../../include/resona/platform.h)). Copy this folder to start your own.

- `src/factory.c`: pages, encoders, presets, settings, Global EQ, Encoder 2 mapping, status
  light. Everything the box plays belongs to the platform; this file keeps mirrors of it for
  drawing and changes it through the platform table.
- `src/renderer.c`: the pixel-art pages (320 x 172, RGB565) and the meters.

In Creator Lab choose **New Project → Box App → Resona factory UI**. Edit the copied
sources, use **Build** for an interactive UI preview, and **Install** to update the box
without a CLI. Desktop preview keeps settings/presets locally and does not process audio.

For scripts:

```bash
resona-lab app build <copied-project>
resona-lab app install <copied-project>
```

Encoder 1: turn moves, click confirms or edits, double click saves, long press goes home
(on an effect of the main page it reorders the factory chain). Encoder 2: turn changes the mapped parameter; press
and long press run the actions chosen in Settings; long press on a parameter maps Encoder 2
to it. An RPK with its own screen hands both encoders to the pack, except
Encoder 1 long press and double click.

Settings also holds **BLOCK SIZE** (64, 128, 256 frames): larger blocks give algorithm packs
more time per call and add latency (2.7 ms at 128, 8 ms at 256, round trip). The box restarts
its audio for a moment when it changes.

A preset is the factory chain (Gate, Comp, Resona, NAM, Cab, each once, any order) or one
stored RPK effect; the EFFECT choice switches between them
(Creator Lab's `docs/preset-chains.md`). Saving goes through a program
job (`program_submit`) and updates only the chosen destination. If storage is full, the UI shows
STORAGE FULL and leaves the preset unsaved; other presets are kept. Apps should handle
`RESONA_ERR_FULL` and must not display success on a negative result. Model browsing follows the
selected effect. The displayed model is adopted only after success; refused changes retain the
previous selection without an error toast. CPU admission and
NAM model-memory management stay in the platform firmware, so an App update alone
does not change them. Built-in effects stay in the platform firmware.

## Home screen and effect icons

The home screen keeps the meter/preset/volume/settings header and draws pedal artwork for the
factory chain's effects or the preset's RPK. RPK pedals have filled pixel-art faces; short pack
names use large lettering and longer names wrap inside the label. Resource counters belong in
Lab's Device monitor, not the home screen.

This is editable Box App code in [src/renderer.c](src/renderer.c):

- `draw_pack_art` draws an RPK effect's pedal and name.
- `draw_user_pedal` supplies the body, knobs, LED and footswitch.
- `draw_effect_art` selects the built-in pedal artwork. Its pixel data is in
  [src/generated/box_ui_pedal_assets.inc](src/generated/box_ui_pedal_assets.inc).

Change those functions, draw your own pixels or replace the artwork arrays,
then preview/build/install your Box App. No platform firmware changes are needed.
An algorithm pack's `ui_draw` controls its editor screen; the module ABI has no home-icon
callback or icon metadata. A Box App can choose icons itself from the effect's name and source
in `program_chain` (`resona_chain_view`).
