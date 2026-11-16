# DSP Tools

Copy this project outside the SDK before building. It compares exact dot products
(including the signed minimum corner case) and feedback rings at init, then
profiles a4096-element dot product while passing mono audio through.

`CHECK=1` means the equivalence checks passed. Change `NATIVE` between0 (portable)
and 1 (optional native backend) and compare the whole-chain CPU in Lab's Device page or
`resona-lab chain status` (Lab currently has no command that reads the named profile
regions back). Both modes must pass the same CPU admission budget. Desktop preview uses
the portable path. No instruction, register or chip configuration is needed.

See [DSP tools](../../agent/docs/dsp-tools.md) for limits, alignment, lifetime,
profiling overhead and the difference between wall time and CPU-cycle counters.
