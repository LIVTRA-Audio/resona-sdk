# Memory Delay

A mono delay with a 1 KiB fast SRAM workspace and a 256 KiB PSRAM delay line.
Create a copy outside the SDK, then build and preview it in Creator Lab. Import the
`.rpk` into a spare preset to test it on a Resona Box.

Change each request's `region` to choose placement. Fast SRAM is useful for hot
working data; PSRAM is useful for large delay lines/tables. There is no automatic
fallback: a request that does not fit rejects loading and leaves the running
effect intact. `resona-lab pack info` lists the requests; Device reports whole-chain
fast RAM and PSRAM.
CPU qualification uses actual placement; preview timing is desktop timing only.

Requests have unique nonzero IDs, nonzero bytes, and alignment16,32,64 or128.
Buffers are zeroed before init and remain valid until the instance is unloaded.
Use `resona_buffer_find(host->memory,id)` during init and retain the returned data
pointer in state. Neither process nor reset allocates memory. Reset clears the
line with a bounded loop; large initialization work still has a device deadline.
A buffer cannot access another instance, the factory firmware or models. Bounds
between buffers in the same arena remain the author's responsibility.

The state block stays in fast SRAM (see Shared State Delay for PSRAM state). The
1 MiB/4 MiB constants are request sanity bounds, not available user budgets: the
platform, factory engines, stacks and existing instances also consume memory.
