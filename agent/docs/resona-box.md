# The Resona Box

A guitar effects box with a screen, two push-encoders, a status light, and USB and wireless
audio and MIDI. Write against the Resona APIs; the platform's internals are private.

- A preset is either the factory chain (Gate, Comp, Resona, NAM, Cab, each once, in any
  order) or one effect on its own. An App that wants its own combination builds an App
  effect graph.
- Read the sample rate, block size and channel count from the host, and the screen size from
  the API (`ui->width`/`ui->height`, or `device_info` in an App); never assume them.
- The Box measures each effect's CPU when it first loads and refuses one that does not fit;
  an effect that later runs over is bypassed. See [CPU qualification](cpu-qualification.md).
- Memory placement for effects: [DSP memory](dsp-memory.md). Lab's Monitor shows what is
  free on a Box.

## Two kinds of user code

| | Algorithm pack | Box app |
| --- | --- | --- |
| What | sound processing: one pack is a whole preset | the whole UI and operation logic |
| API | `resona/module.h` | `resona/platform.h` |
| How many | one per preset (any number stored) | one per box |
| Install | `resona-lab presets import` | `resona-lab app install` |
| Runs | in the audio thread, every block | on the UI thread, ~30 frames a second |
| Rules | strict real-time (see packs.md) | must not block its thread (see box-apps.md) |

Packs are built for a family of Resona devices, not only this box: they take the sample rate,
block size and channel count from the host, so the same pack can run on later devices.

## Talking to a box

### Package target

Algorithm and Box App packs use the Resona-specific `RBX1` target. SDK, builder
and installed firmware must match. Older target IDs have no alias: rebuild old
packs and preset pack dependencies from source before using the updated runtime.
New packs require the matching firmware update. Updating headers does not flash,
migrate or erase any device storage.

`resona-lab` finds a box connected over USB (USB-MIDI). A box needs the Resona platform firmware
(platform API 1) to accept packs and apps.

