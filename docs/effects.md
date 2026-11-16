# Effects

An effect is one sound: a filter, a delay, a drive, a synth voice. It is built into one
`.rpk` file. On the Box a preset plays either the factory chain (Gate, Comp, Resona, NAM,
Cab) or one effect on its own, so your effect has the CPU to itself. To combine sounds, put
several DSP blocks inside one effect.

## The parts

A new effect project (**New Project → Algorithm → Effect**) has:

| File | What you change there |
| --- | --- |
| `src/params.h` | the knobs: name (up to 12 characters), minimum, maximum, default, unit |
| `src/dsp.cpp` | the sound: `dsp_init`, `dsp_param` (a knob changed), `dsp_process` |
| `src/ui.cpp` | the effect's own screen on the Box, and what the encoders do there |
| `src/main.cpp` | the glue to the Box; you rarely change it |

```cpp
// params.h: one knob
const resona_param kParams[P_COUNT] = {
    {"GAIN", -24.0f, 24.0f, 0.0f, "dB"},
};

// dsp.cpp: the sound
void dsp_process(Dsp &d, const float *in, float *out, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; ++i) out[i] = d.gain1.process(in[i]);
}
```

That is the whole of `examples/gain`. Under the template is `resona_module` in
`include/resona/module.h`: your effect's name, parameters, the size of its state, and the
functions `init`, `reset`, `set_param` and `process`.

## Audio

- Mono in, mono out.
- The host tells you the sample rate and the most samples one `process()` call gets; the
  block size can change, so never assume one.

## Rules for `process()`

`process()` runs many hundreds of times a second, so it must be quick and predictable:

1. No `malloc`/`new`, `std::vector` or `std::string`. Keep what you need in `State`.
2. No C library: use `<resona/math.h>` (`resona_sinf`, `resona_expf`, `resona_tanhf`…),
   which gives the same results on your computer and on the Box.
3. No printing, files, waiting or locks, and every loop has a fixed bound.
4. Never output NaN or infinity: guard divisions and clamp feedback.
5. `reset()` clears all history; the same input must then give the same output.

Work out coefficients when a knob changes (`dsp_param`), not in `process()`.
`resona-lab check` tests these rules on your computer.

## Building blocks

`include/resona/blocks/` has ready parts you can combine: oscillators, LFOs, noise,
filters, EQ, delays, chorus, flanger, phaser, reverb, drive, fold, crush, compressor, gate,
envelopes, mixers and meters. Each has `init()`, `reset()`, setters in real units
(`set_rate(hz)`) and `process(x)` per sample. The Flow page in Lab shows how they connect.

## The effect's own screen

`ui_draw` draws the effect's page on the Box (`ui->width` × `ui->height`) about 20 times a second, and
`on_event` gets the encoder turns and clicks. Both run outside the audio. Read your state,
never change it there, and keep them short: one that takes too long is stopped and the
effect is bypassed. The template's `ui.cpp` draws a knob for each parameter;
`<resona/widgets.h>` has knobs, faders, level meters, toggles, step tiles and an XY pad.

## Memory

- `State` is your effect's memory. It is in fast memory unless you ask for the larger,
  slower PSRAM (`examples/shared_state`).
- Large buffers, such as delay lines, can be declared in a memory profile and placed in
  fast memory or PSRAM (`examples/memory_delay`). The Box sets them up before `init`.
- Lab's **Monitor** shows the Box's CPU, fast RAM and PSRAM; so does `resona-lab chain status`.

## CPU

The Box measures each effect the first time it loads and keeps a margin on top. It refuses
an effect that does not fit and says so.
An effect that later runs over its measured cost is bypassed and reported.

A synth or a delay may be quiet when measured. A CPU profile (`examples/cpu_workload`) lets
you play its heaviest case during the measurement, or set a minimum.

Your computer's timings say nothing about the Box: check CPU on the Box itself.

## Try it, then install

| Step | In Lab | Terminal |
| --- | --- | --- |
| Build and check on your computer | **Build** | `resona-lab build`, `resona-lab check` |
| Hear it | **Flow → Listen** | `resona-lab render --param 0=12` |
| See it in the factory App | the Simulator | `resona-lab app preview` |
| Put it on the Box | **Install on Box** | `resona-lab pack build`, then `resona-lab presets import` |

On the Box, choose it in a preset's **EFFECT** tile and save the preset.

More: [the detailed effect guide](../agent/docs/packs.md), [DSP helpers](../agent/docs/dsp-tools.md),
[memory](../agent/docs/dsp-memory.md) and [CPU](../agent/docs/cpu-qualification.md).
