# Splash screens and USB names

Part of platform API 1; no extra firmware is needed.

## USB names in your code

USB naming is part of the Box App source; there is no naming-settings window.
Use this inside your init callback after retaining the platform pointer:

```cpp
#include <resona/startup_helpers.h>

const resona_usb_names names = {"My Synth", "My Synth Audio", "My Synth MIDI"};
int result = resona_startup_usb_apply(platform, &names);
if (result < 0) return result;
if (result == RESONA_STARTUP_REBOOT_REQUIRED) {
    platform->reboot(); // The same code returns0 after the new profile is active.
}
```

See [USB Names in Code](../../apps/usb_names/README.md) for a complete project.
The helper compares saved names before writing. Users never enter a VID/PID;
the platform and Lab look after the USB identity.
Pass NULL to restore factory names. Names use1..47 UTF-8 bytes without control
characters. The whole composite device reconnects, so audio/MIDI software may
need its ports selected again. Named profiles and splashes persist independently
of the Box App. Host code cannot assume all software displays a single string:
OS/DAW labels may combine product/interface/jack names or retain user aliases.
Legacy Windows MIDI naming needs separate hardware qualification.
The bootloader recovery identity stays fixed. Identification is not authentication.

## Splash screens in your code

Splash configuration also belongs in Box App code; there is no dedicated setup
window. Build frames using resona/splash.h, then call:

```cpp
int result = resona_startup_splash_apply(platform, animation, bytes);
if (result < 0) return result;
if (result == RESONA_STARTUP_REBOOT_REQUIRED) platform->reboot();
```

The helper validates the complete asset and compares its stored size/CRC. It
writes only a change; the same init code returns0 after the asset is installed,
avoiding repeated writes or reboot loops. NULL/0 restores the factory splash.
If applying USB names and a splash together, stage both successfully before
requesting a single reboot. Call these helpers from init/control code, never audio.

See [Splash in Code](../../apps/splash_code/README.md) for a complete project. Its
make_animation() function draws four moving-bar frames using bounded RGB565 runs.
Edit colors, timing, duration and drawing code, then Build/Install from Lab.
No external asset editor, JSON file, CLI or setup window is required.
The host preview models these settings and shows the App UI; full early-boot
playback is checked by restarting the device.

Saved data is independent of the installed Box App and takes effect next boot.
No presets, models, saved algorithms or Box App storage are erased. Long animations
postpone Box App initialization/preset recall; USB management remains available.
Duration0 skips; positive durations can reach2147483647 ms. At most128 frames
fit in a24KiB asset. The protected early renderer plays validated data, not arbitrary
user drawing callbacks. Invalid stored data falls back to factory behavior.

## Generate from code

Use the standalone standard-library Python tool:

```sh
python3 tools/splash.py assets/startup/pulse.json pulse.rsp
```

The JSON format has duration_ms and frames. Each frame has hold_ms (16..60000),
a background color and optional rects (at most256). Rectangles use integer x/y/w/h
inside320x172 pixels and #RRGGBB colors. Frame data compresses as RGB565 runs;
complex artwork may exceed the asset capacity even with a short duration.
The public binary format is documented in `include/resona/startup.h`. Your own
tools may generate it; Creator Lab and firmware both validate the complete asset.
No Pillow, Qt or firmware source is needed to run the packer.

## Box App API

Check platform version and table size before using startup_info, startup_usb_set
and startup_splash_set. They run as checked control calls, never in audio processing.
USB setters accept names only, assign/reuse a managed profile, and apply next reboot.
A NULL names pointer restores factory naming. Splash data is copied and validated
before committing; NULL/0 restores the built-in animation. Invalid user pointers
fault within the isolated domain; no direct flash/descriptor access is exposed.
See [Startup Studio](../../apps/startup_studio/README.md) for explicit MIDI-triggered
examples. Preview changes are local to its worker and do not rename a real USB device.

A box keeps a limited number of custom name profiles. Creator Lab gives each one its
USB identity when the names are applied, and refuses when none is free.

Managed Box App installation skips the current splash after verifying the same
unit identity, so a long authored duration does not delay installation verification.
Normal power-up and explicit Restart retain the authored duration. The CLI also
provides `startup skip-splash` for this boot only; it does not change stored assets.

## Bluetooth names in code

Use `<resona/bluetooth_helpers.h>` and declare
`resona_bluetooth_names names={"My Audio","My MIDI"};`.
`resona_bluetooth_apply(platform,&names)` returns0 if active,1 if saved but a
full power cycle is needed, or a negative error. NULL saves product defaults.
Neither helper nor platform automatically restarts Bluetooth or the device.
Enable Bluetooth before applying; if busy/off/unavailable, retry from a later
control event. The portable preview models names locally and never accesses radio hardware.

Names use1..31 audio /1..28 MIDI UTF-8 bytes without control characters.
Both are persisted together; unchanged values do not write flash. Read
`platform->bluetooth_names_get(&info)` to compare `active` and `saved` names.
Lab shows a notice while `power_cycle_required` is true. Disconnect USB-C **and
any9 V supply**, then reconnect. Restarting only the main processor does not apply
these settings. Host Bluetooth name caches may require a refresh/reconnection.
Pairing addresses/keys, MIDI service and recovery identities remain platform owned.
Bluetooth/USB/DSP sample-rate selection is not exposed by this API.

See [Bluetooth Names in Code](../../apps/bluetooth_names/src/app.cpp). Shared automation
uses `resona-lab bluetooth status`, `bluetooth names --audio NAME --midi NAME`
and `bluetooth restore-names`; these are diagnostics, not a separate setup window.
