# Shared State Delay

A 131072-sample delay (about 2.73 seconds at 48 kHz) with 512 KiB of history
inside its main state. The module's optional `state_region` tail selects
`RESONA_MEMORY_PSRAM`. Zero or `RESONA_MEMORY_FAST` keeps state in fast SRAM.

Build and preview a copied project outside the SDK. State and explicitly
placed buffers share the live SRAM/PSRAM budgets. Runtime
stacks, metadata, platform reserves, fragmentation and the previous instance
also consume memory; a declaration is not a reservation or guaranteed fit.

Try the pack in a spare preset (`resona-lab presets import`); do not overwrite a preset you need.
The first delayed audio appears after 131072 frames. CPU qualification and
isolation apply to the actual placement. There is no audio-thread allocation
or automatic placement fallback. Reducing MIX to zero gives the dry signal.
