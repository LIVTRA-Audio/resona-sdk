# Getting started

You need **Creator Lab** and, to install, a **Resona Box** on a USB data cable. Lab comes
with this SDK and the compiler for the Box. The Simulator also needs a C++ compiler on
your computer: on macOS, the Command Line Tools (Lab offers to install them); on Windows,
Clang or GCC. Lab's **Settings** lists these tools and opens that list when one is missing;
`resona-lab doctor` checks them too.

## Your first effect

1. **New Project → Algorithm**, choose the **Effect** template, give it a name and a new
   folder. Lab opens `src/`:
   - `params.h`: the knobs (name, range, default, unit),
   - `dsp.cpp`: the sound,
   - `ui.cpp`: the effect's own screen on the Box.
2. **Build.** The Simulator plays your effect inside the Box's factory App, as if a preset
   used it. Turn the knobs in the Simulator to change its parameters.
3. **Listen.** Open **Flow** and press **Listen** (Space) to hear your effect on your
   computer.
4. **Install on Box.** Lab compiles it for the Box and stores it there.
5. **On the Box**, open a preset's **EFFECT** tile and choose your effect. Save the preset
   to keep it.

## Your first App

1. **New Project → Box App** and a template:
   - **blank**: the smallest App,
   - **factory**: the Box's real software, to change or learn from,
   - **synth**: an App that plays an effect as an instrument,
   - **RGB** and **Bluetooth names**: small Apps that show one feature each.
2. **Build.** The Simulator runs your App: the screen and both knobs work as on the Box.
3. **Install on Box.** The Box restarts into your App. Your presets, effects and models
   stay as they are.
4. **To go back**, install the factory App again (a new project from the **factory**
   template, or `apps/resona_factory`).

If an App fails to start, the Box shows **safe mode** and says why; you can still install
another App. Holding **Encoder 1 while powering on** also starts safe mode.

## Learn from the examples

For effects, **New Project → Algorithm → From an example** copies one of `examples/` into
your own folder. For the Apps in `apps/`, copy the folder yourself and open the copy in
Lab. Change the copy, not the SDK.

## The same from a terminal

```sh
resona-lab new ~/Documents/"Resona Lab"/my-fx --name "My FX"     # an effect
resona-lab build my-fx            # build and check it on your computer
resona-lab render my-fx --param 0=12   # write build/render.wav
resona-lab check my-fx            # stress checks; run before installing
resona-lab pack build my-fx       # build/<id>.rpk for the Box

resona-lab new my-ui --name "My UI" --template app-blank   # an App
resona-lab app build my-ui
resona-lab app preview my-ui --out screen.png
```

All commands: [tools](../agent/docs/tools.md).

## Projects from someone else

Lab asks you to **Trust** a project before it builds or runs its code, because the
Simulator runs that code on your computer. Read a project you received before trusting it.

Next: [Effects](effects.md) or [Apps](apps.md).
