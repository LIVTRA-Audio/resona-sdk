# Flexible code placement

Build and try this project through Creator Lab or `resona-lab pack build` and
`resona-lab presets import` (a pack is a whole preset).
It uses a large lookup table, too big for the fast code pool, with a simple mono level
processor.

The `code` descriptor selects `RESONA_CODE_PSRAM`. Select `RESONA_CODE_SRAM`
to charge the shared SRAM heap instead. `RESONA_CODE_FAST` uses the dedicated
pool and rejects this oversized image. There is no automatic fallback.

Executable
code/constants are read-only in the user domain; state/buffers remain writable
in their separately declared regions. CPU admission measures the chosen region.
PSRAM can be slower and is not a promise that a CPU-heavy algorithm will fit.

`resona-lab pack info` shows the code region and image bytes. The Device page's whole-chain
SRAM/PSRAM usage includes heap code; the fast code pool only counts default placement. Actual
fit includes stacks, reserves, alignment, upload/retained package bytes, other
algorithms and the previous instance during replacement. Flash storage is a
separate smaller budget: a RAM-loadable algorithm may not fit saved storage.
Saving must refuse full storage without deleting existing presets.
