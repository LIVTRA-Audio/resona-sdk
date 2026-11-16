# CPU Workload

A Tiny Synth derivative demonstrating optional desktop CPU-profile stress hooks.
The ordinary preview starts silent. Lab check can exercise an active low note,
a high-note/waveform transition and a release tail. Polyphonic projects should
exercise their maximum voice workload during development.

On the matching1.02 device runtime, `prepare` and `min_permille` are not used for
CPU admission. No import-time benchmark or reservation occurs. Exercise the
real App/algorithm and watch Creator Lab's USB console. The DSP budget is
enforced as a256-block average; brief peaks within the physical deadline
are allowed. See [runtime budget](../../agent/docs/cpu-qualification.md).

The optional module tail remains in the public layout; packs without it load.
