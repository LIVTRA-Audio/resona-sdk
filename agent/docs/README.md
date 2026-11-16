# Resona developer guides

Copied from the Resona SDK into every project (read-only; `resona-lab sdk sync .` refreshes
them together with the project's SDK pin; legacy local copies remain supported).

| File | For |
| --- | --- |
| [resona-box.md](resona-box.md) | the device: audio, block sizes, CPU and memory limits, screen, encoders |
| [cpu-qualification.md](cpu-qualification.md) | target CPU trials, workload hook and reservation floor |
| [dsp-tools.md](dsp-tools.md) | portable/native DSP kernels and named device profiling |
| [dsp-memory.md](dsp-memory.md) | choose fast SRAM or PSRAM buffers, lifetime and admission |
| [packs.md](packs.md) | algorithm packs: the module API, real-time rules, screens, the UI kit |
| [box-apps.md](box-apps.md) | box apps: how they run, the platform API by topic |
| [ui-libraries.md](ui-libraries.md) | optional App UI libraries: Box UI widget kit and the LVGL 9 port |
| [startup.md](startup.md) | USB names, Bluetooth names and the start-up animation, set from App code |
| [contracts.md](contracts.md) | the detailed rules behind each API: versions, memory, graphs, presets, errors |
| [tools.md](tools.md) | the `resona-lab` commands and the MCP server for AI agents |
| [recipes.md](recipes.md) | worked changes for packs and apps |

The short, plain guides are in the SDK's `docs/` folder: getting started, effects, Apps,
the reference table and SDK versions.

A preset is the factory chain or one RPK. Put a pack on the box with
`resona-lab presets import --request '{"preset":N,"path":"build/<id>.rpk","run":true}'`; the
device measures and gates resource use before it plays. `chain status --json` reports live
whole-chain CPU/RAM/code resources (no per-effect costs).
