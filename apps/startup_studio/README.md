# Startup Studio

Copy this project outside the SDK and open/build it in Creator Lab. Uses
platform API 1. Initialization only reads settings; it does not change presets,
algorithm storage or Box App data.

On the USB musical MIDI port, send channel1 control changes with value127:

| CC | Action |
| --- | --- |
| 80 | Save example USB names for next reboot; platform assigns the PID. |
| 81 | Save a two-frame5-second splash for next reboot. |
| 82 | Restore the factory splash. |
| 83 | Restore factory USB names for next reboot. |
| 84 | Reboot and apply startup settings. |

These settings persist independently of this Box App. Use code-based helpers to apply more detailed animations or restore defaults.
The host preview models calls in memory only; it never changes a USB device.
For automatic setup in init, see [Splash in Code](../splash_code/README.md).

For automatic names declared directly in init code, use the checked helper in
[USB names in your code](../../agent/docs/startup.md#usb-names-in-your-code). It returns a reboot-required result once, then returns0 after the profile is active.
