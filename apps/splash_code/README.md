# Splash in code

Copy this project outside the SDK and open it in Creator Lab. Edit duration_ms,
frame_ms, colors and make_animation() in src/app.cpp, then Build/Install.
Uses platform API 1.
There is no splash setup window.

The public builder creates320x172 frames in a caller-owned buffer. Each run
adds count row-major RGB565 pixels; a frame must cover exactly55040 pixels.
Frames loop for the declared duration. Duration0 skips the startup animation.
The builder enforces frame, size, timing and memory bounds without allocating.

resona_startup_splash_apply compares the stored asset's complete size/CRC first.
It writes only a changed, fully validated animation and returns1 when a reboot
would apply it. After that reboot the same init code returns0, avoiding repeated
flash writes or reset loops. Negative returns leave the old asset installed.
Pass NULL/0 to restore the factory splash from code.

The protected renderer plays saved data before the Box App starts. Drawing
code generates those frames during App initialization; arbitrary App code does
not execute in early boot. Managed installation may skip playback for startup
verification, while the next normal restart plays the full authored animation.
No preset, model, algorithm, settings or Box App storage is modified.

Host preview models the setting calls and shows the App UI; it does not replay
the early-boot animation. Use a normal device restart to see that stage.
