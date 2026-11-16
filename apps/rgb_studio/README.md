# RGB Studio

Box App example (platform API 1). Open a copy in Creator Lab, trust reviewed source, build and
preview it, then install it. It only controls the light and
screen; it never changes presets, algorithms or saved settings.

Top encoder chooses red/green/blue/brightness/animation/period; bottom encoder edits.
Bottom long press releases custom light control; USB CC22 does the same.
Bottom click cycles solid, blink, breathe, rainbow and one-shot fade. Values are
linear intensity0..255. Blink/fade use the secondary color (blue in this example).
Rainbow uses its own hue wheel; breathe uses a triangular brightness envelope.
Custom curves/sequences can issue colors from UI frame/timer events.

USB MIDI CC16/17/18/19 map0..127 to red/green/blue/brightness0..255; CC20 selects
animation modulo5; CC21 sets period20+100*value milliseconds. This demonstrates
controlling RGB through MIDI using only public APIs.

The physical light is shared: Box Apps own it through checked calls; individual
DSP packs cannot contend for it. A deliberate Lab override blocks app writes with
RESONA_ERR_STATE until released. NULL to rgb_set restores legacy status policy.
Fault recovery reclaims the light. No GPIO/register or private platform API is used.
