# Resona SDK

Make your own sounds and your own software for the **Resona Box**, in C or C++, with
**Creator Lab**. Try everything in Lab's Simulator, then install it on the Box over USB.

## Two kinds of project

| | An **effect** | An **App** |
| --- | --- | --- |
| What it is | A sound: a filter, delay, drive, synth… one `.rpk` file | The Box's whole software: screens, both encoders, presets, settings, MIDI |
| How it plays | A preset is the Box's factory chain or one effect; choose your effect on the Box | One App runs at a time; the factory App is one too, and its source is here |
| In Lab | **New Project → Algorithm** | **New Project → Box App** |
| Learn more | [Effects](docs/effects.md) | [Apps](docs/apps.md) |

## Quick start

1. Install Creator Lab and connect the Box with a USB data cable.
2. In Lab, choose **New Project**, then **Algorithm** or **Box App**, and a template.
3. Edit the code and choose **Build**. The Simulator shows it on the Box's own screen;
   turn and click the two knobs to try it.
4. Choose **Install on Box**. An effect is stored on the Box: pick it in a preset's
   EFFECT tile. An App replaces the Box's software and starts at once.

Step by step: [Getting started](docs/getting-started.md).

## What is in this folder

| Folder | What |
| --- | --- |
| `include/resona/` | The API: `module.h` (effects), `platform.h` (Apps), `program.h` (presets), DSP blocks, UI |
| `templates/` | The starting points Lab offers for new projects |
| `examples/` | Effects to learn from: gain, drive, delay, Tiny Synth and more |
| `apps/` | Apps to learn from, including the Box's own factory App |
| `lib/` | Optional App libraries: `widgets` and an LVGL port |
| `device/`, `host/` | Small runtime pieces Lab links into your project, and the Simulator's model of the Box |
| `agent/` | Detailed guides Lab copies into each new project, for you and for AI coding assistants |
| `tools/`, `tests/` | Source checks, a splash packer and the SDK's own tests |

The Box's own firmware, its built-in effects (Gate, Comp, Resona, NAM, Cab) and its
models are not part of the SDK; your code reaches them through the public API.

## Docs

- [Getting started](docs/getting-started.md): your first effect and your first App
- [Effects](docs/effects.md): writing a sound
- [Apps](docs/apps.md): writing the Box's software
- [Reference](docs/reference.md): headers, errors and API limits
- [SDK versions](docs/sdk-versions.md): versions, sharing projects, downloading an SDK
- [Detailed guides](agent/docs/README.md): every API in depth

## Version and license

This is SDK **1.0.0** (`sdk.json`); changes are in [CHANGELOG.md](CHANGELOG.md).

Copyright (c) 2026 Livtra Technology Co., Ltd. Licensed under the GNU General Public
License v3.0 (GPL-3.0): see [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md).
Effects and Apps built with this SDK are covered by the GPL too: if you share or sell one,
share its source code under GPL-3.0.
