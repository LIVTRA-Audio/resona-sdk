# USB names in code

Copy this project outside the SDK. Edit the `names` constant in src/app.cpp,
then Build/Install it as a Box App in Creator Lab. Uses platform API 1.
There is no naming settings window and no VID/PID to enter.

The checked helper compares names with persistent settings. It requests one
additional reboot only when a different profile needs to become active. The
same source then starts normally without rewriting flash or looping reboots.
Lab reconnects through reserved CDC management and verifies the same physical unit.
No preset, model, algorithm, settings or Box App storage writes are made.

Names persist independently of the installed App. To restore factory names,
replace `&names` with `NULL` in the helper call and install that code, or use
`resona-lab startup restore-names`. If an App still declares other names, it
will reapply those names at its next initialization; replace that App first.

Preview models settings locally and reports reboot as an unsupported host
operation; it does not rename the computer's USB devices. Actual OS/DAW labels
may combine product and MIDI jack names. Windows qualification is separate.
