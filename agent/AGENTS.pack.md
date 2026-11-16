# @NAME@ — an algorithm pack for the Resona Box

You are working on a Resona **algorithm pack** (`@ID@`): sound processing that is a whole preset on
a Resona Box: a preset is either the factory chain (Gate, Comp, Resona, NAM, Cab) or one RPK. It
is built into a `.rpk` file and stored in the box's user area.

## Read first

- `docs/resona/packs.md`: the module API, the rules for `process()`, parameters, screens,
  input, the UI kit. **Follow its rules; the checks enforce most of them.**
- `docs/resona/resona-box.md`: the device (rate, block sizes, CPU limits, screen, encoders).
- `docs/resona/tools.md`: every command, and the MCP tools if your client has them.
- `docs/resona/recipes.md`: worked changes (add a parameter, a filter, a screen page).
- `resona-lab sdk status . --json`: find the exact read-only SDK headers used by this project. New projects use a shared version/hash pin; bundled and legacy projects may have `sdk/`.
  `module.h` is the contract; `math.h` replaces libm; `blocks/` has ready DSP parts.

## The project

- `resona.project.json`: name, id, version, sources (@SOURCES@).
- Sources are C++17, built twice: for the desktop (preview, checks) and for the Resona device runtime.
- `build/` is output; never edit it.

## The loop (run these yourself; they print what went wrong)

```sh
resona-lab build .            # compile, forbidden-symbol check (--json for machine output)
resona-lab check .            # stress checks: finite, bounded, deterministic, reset
resona-lab render . --param 0=12          # audio through it -> build/render.wav, peak/rms
resona-lab ui-preview . --scale 2         # its screen -> build/ui.png (look at it)
resona-lab pack build .       # the device pack -> build/<id>.rpk
resona-lab presets import --request '{"preset":50,"path":"build/<id>.rpk","run":true}'  # store it as preset 50 and play it
```

`check` must pass before anything goes to a box. Look at `build/ui.png` after changing the
screen, and listen to (or measure) `build/render.wav` after changing the sound.

## Hard rules (short version)

No heap, exceptions, RTTI, C library, I/O or locks in a module; every loop bounded; never
emit NaN/Inf; state lives in `State` (no mutable globals); `process()` handles any frame
count from 1 to `host->max_block_frames`; compute coefficients in `set_param()`, not per
sample; parameter names at most 12 characters.

## README.md is the public page

`README.md` is uploaded with every release and shown as this project's page in Creator Lab.
Write it for players, not developers; developer notes go in `DEVELOPING.md`.

- When you add, remove or rename a parameter or control, or change what it does, update the
  **How to use it** and **Controls** sections of `README.md` in the same change.
- Add a line under **Changes** for each new version (match the version in `resona.project.json`).
- Images go in `images/` and are linked with a Markdown image link to a relative path such as `images/screen.png`
- `images/cover.png` (or `.jpg`/`.webp`) is the cover shown on the project card and page; Upload sends it
  (PNG, JPEG, WebP or GIF, at most 8 MB each, up to 20). They are uploaded with the README.
- Keep it honest: describe only what the code does. No external image links; they are not shown.

## Do not

- Edit cached/bundled SDK files or `docs/resona/`. Use an explicit `resona-lab sdk sync .` to update this project; other projects keep their pins.
- Commit `build/`.
- Write to a box (`presets import`, `presets save`) or upload (`pack upload`) unless the user asks.
