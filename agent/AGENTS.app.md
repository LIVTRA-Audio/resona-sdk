# @NAME@ — a box app for the Resona Box

You are working on a Resona **box app** (`@ID@`): the box's whole user interface and
operation logic — screen, encoders, presets, MIDI and SysEx handling, settings. The
platform (closed firmware) keeps audio, drivers, storage and safety; the app reaches
everything through the function table in the resolved SDK's `include/resona/platform.h`. One app is
installed on a box at a time.

## Read first

- `docs/resona/box-apps.md`: how an app runs, the platform API by topic, the rules.
- `docs/resona/resona-box.md`: the device (screen, encoders, blocks, flash, limits).
- `docs/resona/tools.md`: every command, and the MCP tools if your client has them.
- `docs/resona/recipes.md`: worked changes (a new page, a setting, a MIDI mapping).
- The resolved SDK's `include/resona/platform.h`: the exact API (read-only); use `resona-lab sdk status . --json` for its location. `ui.h` draws shapes and text,
  `libc.h` lists the few C functions an app has.

## The project

- `resona.project.json`: name, id, version, `"kind": "app"`, sources (@SOURCES@).
- C and C++17 sources, built for the Resona device runtime into `build/<id>.rpk`.
- `build/` is output; never edit it.

## The loop

```sh
resona-lab app build .        # compile and link for the box (--json for machine output)
resona-lab app install .      # build and install on a connected box; it reboots into the app
resona-lab app info           # what the box runs now (the app, or safe mode and why)
resona-lab app reboot --safe  # one boot in the platform's safe-mode UI
```

Preview the whole UI with `resona-lab app preview`, and build often (the compiler
catches most mistakes). Validate device-dependent behavior on a box. If the box comes up in safe mode, your app crashed
while starting: read `app info`, fix, install again (installing always works in safe mode).

## Hard rules (short version)

- `init`, `event` and `frame` run on the app's main thread and must return quickly; never
  wait in them. Use `timer_start` for periodic work. Background threads and RTOS mutexes are
  unsupported in the protected runtime; keep work bounded and event-driven.
- Draw into the back buffer from `screen_begin`, then `screen_present` the changed rectangle.
- Use `P->alloc` (counted against `heap_bytes` in your `resona_app`), not `malloc`; the app
  links no C library beyond `resona/libc.h`.
- Check every platform return value (`RESONA_OK` or a negative `RESONA_ERR_*`).
- Reserved SysEx ops belong to the platform; you cannot send or receive them.

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
- Install on a box or clear its data unless the user asks.
