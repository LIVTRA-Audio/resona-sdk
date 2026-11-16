# DSP runtime budget

The matching Resona Box 1.02 runtime enforces CPU limits while real audio runs.
It does not benchmark an algorithm on import, run CPU-profile hooks on-device,
reserve predicted CPU time, or requalify a chain when models or routes change.
Package validation, protected initialization, memory/code limits and graph
validation still apply. Installing headers alone does not update device firmware.

## Two limits

- Sustained DSP: the rolling mean of the latest256 completed audio blocks must
  stay at or below100% of the DSP budget. At64frames/48kHz,
  the window spans about341ms. DSP includes effects, routing and isolation work;
  common post-processing and other platform work are counted separately.
- Per-block execution: each processor receives the remaining physical DSP time
  after measured platform overhead. Protected user code is aborted on a detected
  deadline or protection fault. Trusted built-in processors retain measured
  deadline checks. Routing checks its deadline between routes. Brief DSP peaks
  above100% of the budget are allowed within this physical limit.

No average can predict the next callback. Runtime enforcement detects overload
and contains it; this is not a promise that arbitrary code can never cause a
missed callback. The whole-callback overrun counter includes platform/IRQ work
and is reported separately from effect faults.

On sustained overload the OS bypasses the most expensive executing effect in
that callback and emits an error. It then observes a full new window before
shedding another effect, so old measurements do not immediately trip the rest
of the chain. The history is not reset by changing graphs or parameters.
Deadline, access and non-finite-output failures also bypass the affected effect.
A routing deadline fault stops the App graph and emits silence; the App keeps
audio ownership. Reload/recreate a faulted processor (or commit a repaired graph)
after correcting the problem; there is no automatic retry loop.

## Creator Lab console

Open the USB serial monitor and enable serial output. Once per second the OS
reports usage on the developer budget scale, where **100% is the DSP budget limit**.
For example:

```text
[DSP] budget 62.5% / 100%; peak 110.0%; average 256/256 blocks
[DSP ERROR] #7 My effect: average budget exceeded; bypassed (...)
```

A peak above100% of this budget is permitted if the sustained mean and physical
deadline remain satisfied. Console errors identify a runtime instance and its
name. Logs are queued as bounded records from audio, then formatted and sent
outside the audio callback. The output switch controls both program logs and
these DSP reports. Hardware access details remain private to the OS.

`resona-lab presets cpu` exposes actual DSP, platform and whole-audio timing,
rolling DSP average, `budget_usage_percent` and `budget_peak_percent`. These are
device measurements, not desktop simulator estimates. CLI, MCP and the GUI use
the same core operations and USB log stream on macOS and Windows.

## SDK workload hooks

The optional `cpu_profile` tail remains in the public module layout for desktop
stress/check tooling. The device does not execute its `prepare` hook or charge
its `min_permille` floor. Test real parameter changes, note loads, transitions
and long tails while watching runtime budget and error reports. Desktop timing
does not establish device performance.

## Memory

Use the live fast RAM, PSRAM and code budgets in Creator Lab. Platform code,
factory processing, stacks and buffers consume part of physical memory; heap
figures describe live allocatable memory. Memory allocation failures can still
refuse an import or graph change before audio starts.
