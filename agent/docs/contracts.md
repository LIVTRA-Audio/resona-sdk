# API contracts

The detailed rules behind the API. For the numbers alone, see the [reference](../../docs/reference.md).

## Versions and contracts

| Contract | Current value | Compatibility rule |
| --- | --- | --- |
| Algorithm module ABI | 2 | `struct_size` negotiates optional tails; rebuilt modules must be accepted by the installed runtime. |
| Box App platform API | 1 | One table, no compatibility layers; the OS version is 1 too. Apps built for the old numbering must be rebuilt. |
| Pack file format | 1 | Creator Lab packages the device image and relocation data; do not substitute a raw compiler binary. |
| Device package target | RBX1 | SDK, Creator Lab builder and installed firmware must agree. |
| Audio | read from the host | Read rate, channels and maximum frames from the host rather than assuming the preview layout. |
| Preset | The factory chain (Gate, Comp, Resona, NAM, Cab) or one RPK | Composite DSP is permitted inside one RPK within actual device budgets. |
| App size | at most 512 KB | One App at a time. Lab's build tells you the size and refuses a larger App. |
| Parameters | Bounded by package size, memory and storage | `get_param` reports module UI edits; new snapshots use float32. |

## Package target

Algorithms (`CODE`) and Box Apps (`APP1`) use the product-specific target `RBX1`
(little-endian value `0x31584252`). The SDK and builder emit this ID; the updated
firmware loaders accept only this target, without a legacy alias.

This is a coordinated compatibility change. Rebuild older `.rpk` files from
source with the updated Creator Lab, refresh pinned SDKs when needed, and install
compatible firmware before loading the new packages. Old packages, including
dependencies referenced by existing presets, need rebuilding/replacement before
recall on that firmware. Device / Preset health now diagnoses saved dependencies. After rebuilding an
obsolete algorithm with the same identity and parameter layout, explicit Repair
can replace its shared pool entry while retaining its ID and every preset record.
The old CRC is checked; copy-on-write needs free staging space. Changed parameter
semantics, missing/corrupt source packages and full storage refuse repair. Normal
Memory/package checks still apply on recall. No automatic migration, erase or preset
rewrite is performed. A device still running older firmware will reject the
new target. Header updates alone do not update the physical device.

## Optional algorithm tails

Current ABI2 headers include optional get_param, cpu_profile, memory_profile and
state_region and code_profile tails. The current device descriptor is 88 bytes;
matching-target 68/72/76/80/84-byte prefixes remain loadable. Use resona_host_tools()
to check the host-tools table size and fall back to portable operations when absent.

See [DSP runtime budget](cpu-qualification.md). The matching1.02 runtime
measures actual callbacks and enforces a256-block mean at80%DSP, allowing short
peaks within the physical deadline. It does not qualify imported code or reserve
predicted CPU. The optional cpu_profile tail is used only by desktop tooling.

## Public memory and execution rules

Algorithm state defaults to fast SRAM or explicitly selects PSRAM through the optional
`state_region` tail. State and buffers share live device budgets, with explicit code placement (see below). Platform stacks, buffers, factory effects and other running packs consume
additional memory. Live allocatable budgets are smaller than physical memory
capacities and must be read from Device or `resona-lab chain status`.

Algorithm state is host-owned memory in its declared region. Modules may append a version1
`memory_profile` to declare up to16 additional named buffers in fast SRAM or
PSRAM; see [Memory Delay](../../examples/memory_delay/README.md) and
[`memory.h`](../../include/resona/memory.h). Requests are allocated at load time,
zeroed before init, and released with the instance. No allocation is permitted
in process(). Requests are checked against live free memory and platform reserves
while preserving the previous instance. CPU use is enforced during real audio.
No automatic placement fallback is performed. Zero selects fast
SRAM; `RESONA_MEMORY_PSRAM` selects PSRAM. No fallback between regions occurs.

A pack's total footprint is larger than its own state and buffers (stacks, alignment and
platform bookkeeping), and the package itself takes memory too. Device reports both
buffer layout sizes and arena sizes; global free memory is the admission source.
Preview uses aligned desktop allocations and validates the same requests, but
cannot qualify hardware timing or isolate native desktop code.
Native helpers and opt-in named wall-time profiling are available through the
optional host tools extension; see [DSP tools](dsp-tools.md).
Portable preview uses the same
numerical semantics but does not provide hardware timing or memory protection.

User code accesses the device through published APIs. The protected runtime does
not map firmware, models, peripheral registers or other programs into the
user domain. Isolated Box Apps currently use events/timers; native background
threads and RTOS mutexes are unsupported. Faulted algorithms need reloading; a
faulted Box App falls back to the platform's safe UI.

## Preview scope

Algorithm previews run the user's DSP on the desktop. Box App previews model
drawing, controls, local settings, temporary presets and the local preset chain. They
do not run factory DSP, load real device assets or reproduce USB, wireless or RTOS
scheduling. Preview storage is temporary. Native host previews execute local code;
they are not an OS permission sandbox for untrusted projects.

## What stays separate

The SDK includes the editable factory UI, public DSP blocks and small user-program
runtime helpers. It does not include factory engine implementations, trained
models, platform firmware, drivers, bootloader code, keys or compiler
binaries. Firmware update authentication and platform security remain separate
from this SDK's source license.

## Parameter handoff and desktop containment

Current firmware applies coalesced `set_param` callbacks at the start of an audio
block, inside the same audio deadline/reservation as `process`. Use
[`parameters.h`](../../include/resona/parameters.h) for portable setters and UI edits:
publish values from control; consume/apply them before DSP processing. Do not
mutate DSP coefficients from drawing or event callbacks. Existing hosts may
still call setters concurrently; updating headers does not update the box.

Creator Lab runs native algorithm previews in a disposable worker with bounded
IPC and a two-second per-request timeout. Crashes/hangs terminate that preview,
not the editor. It is not an OS permission sandbox for untrusted projects.

## Algorithm code placement

`code_profile` optionally selects `RESONA_CODE_FAST` (the default fast code
pool), `RESONA_CODE_SRAM` (shared SRAM heap) or `RESONA_CODE_PSRAM` (shared
PSRAM heap). See [Flexible Code](../../examples/flexible_code/README.md).
Code and constants stay read-only and separate from writable state; the platform's own
code and data stay out of reach. CPU admission/deadlines are unchanged and run at the
chosen placement. No silent fallback or extra CPU allowance.

Image4 MiB and package8 MiB+64 KiB are defensive parsing bounds, not available
memory grants. Code allocations round to128 bytes and share actual heap space
with state/buffers, runtime overhead, reserves and other instances. Upload and
retained package copies also consume PSRAM; replacements keep the old instance
until qualification passes. The dedicated pool is reported separately; heap
code is included in SRAM/PSRAM usage. Persistent algorithm storage remains a
file in the shared user area (one pool for Apps, RPKs, models and presets) and refuses
full saves without evicting other presets.

## Status-light RGB

The platform table has rgb_set/rgb_get. RGB is a Box App/UI-thread facility.

Startup customization (splash, USB names) and the portable splash builders and
apply-on-change helpers are described in [startup](startup.md).

Bluetooth audio/MIDI names are persisted, with active/saved readback and a full-power-cycle
notice. Names are UTF-8, 31/28 bytes; changing them does not change pairing identities,
USB profiles, MIDI services, recovery, or sample-rate policy.

## App-owned effect graphs

`effects.h`/`effects.hpp` give an App its own effect-instance graph. One
`resona::Effects` controller begins App ownership, creates built-in or installed
`.rpk` instances, connects named input buses and commits a graph. Effect count is
limited by admitted CPU, RAM and code. While an App owns audio the preset chain is
suspended. Built-in model indices are zero-based within their engine library; automatic
model selection fails if no compatible asset exists. Instances are mono; USB/Bluetooth
buses retain explicit left/right channels. Connections sum at each destination;
parallel paths are scheduled within the existing audio block. Cycles and duplicate
edges are rejected. Use a delay inside a DSP block for feedback.

Creation is synchronous and initializes the processor without a CPU benchmark;
distribute expensive initialization across events. Every resident instance consumes
memory/code, including disabled or unconnected instances. Only real DSP processing
contributes to runtime CPU usage. A failed create/commit preserves the current graph. Destroy
requires removing the instance's live routes first. App faults release its graph
and instances; reinstall/reboot starts new state. Stored pack dependencies stay
pinned while resident. Live preset parameter, on/off and mapping writes from Lab are
refused while an App owns audio. Graphs are runtime state; the App reconstructs them on
startup.

The host preview models built-in metadata/control, handles and graph validation.
It does not run private DSP, load stored device packs, or report hardware admission
measurements. Pack creation returns unsupported in preview. Verify audio and CPU
on the device. `resona-lab app effects` and the MCP tools provide read-only
instance/fault/resource telemetry.

## Recoverable App storage ranges

Preview and firmware return `RESONA_ERR_ARG` for invalid
App-storage offsets/lengths, including overflow. Failed reads do not change the
destination and failed writes do not touch storage. A valid non-NULL buffer with
a zero-length span at the end is a successful no-op. Memory-access violations
remain protected App faults.

## App control limits

The device supports power-of-two App allocation alignment1..4096 and64 live
allocations; invalid requests return NULL. Incoming MIDI events are complete
messages of at most256 bytes; larger messages are dropped, never truncated.
Outgoing MIDI accepts1..512 bytes; invalid scalar lengths/ports return ERR_ARG,
with protected pointer checks retained for valid spans. Preview returns the same
scalar errors but does not simulate MIDI transport. Graph commits validate
structure, memory and ownership without CPU qualification. Runtime sustained
and physical limits apply after activation; overload appears in effect status
and the Creator Lab USB console. Invalid graphs retain the previous graph.

## Events and timers

Timer periods are1..2147483647ms (about24.9days), IDs use all32 bits, and
repeat0 means forever. Invalid periods return ERR_ARG without replacing an
existing timer. Eight live timers are available; a ninth returns ERR_NO_MEMORY.
The firmware samples time after callbacks before checking each timer.

post() copies the event and its valid MIDI/CTRL payload (at most256 bytes) before
returning. Received payloads live only for their callback. The32-entry queue
includes a queued event being dispatched; new posts are dropped when full.
Device delivery shares the queue with other platform events. No reliable-delivery
acknowledgement is implied. Preview owns payloads too, models the64-record
App allocation limit, zeros allocated bytes and returns the same timer errors.

## Character formatting and global EQ

Character formatting honors width and left alignment, including zero characters
and truncated buffers, and returns the full would-have-written length, in the App's
linked snprintf and in P->format.

Global EQ validates the complete request before changing state. enabled and
persist must be0or1; five bands must be-30..30half-dB and level-24..24half-dB.
Invalid arguments return ERR_ARG without changing any EQ field. The shared
resona_global_eq_valid helper is used by preview and native platform code.
Valid persist0 calls change enable, bands and level without saving. Protected
user-buffer checks still apply; this does not permit invalid pointers.

## UTF-8, App fonts and closed circles

Built-in text decodes UTF-8 scalars and uses one small fallback glyph for each
unsupported scalar. Text byte budgets remain; truncation keeps complete scalar
boundaries. Malformed input uses replacement scalars. The text-width helper
counts the same scalars, in native and preview renderers alike.

Exact+/-360-degree dot sweeps exclude the repeated endpoint (count positions
spaced by sweep/count); partial arcs retain both endpoints. This does not
prevent pixel overlap at small radii/large dot sizes.

Apps and block projects can carry their own 1-bit Unicode glyph assets and use
resona/font.h. See the [App-owned Unicode font example](../../apps/unicode_font/README.md).
The helper uses existing public primitives and needs no OS font installation. Font assets count toward the normal project budgets; block
screens retain their draw-call limit. The optional local TTF/OTF converter uses
Pillow for authoring only and copies the supplied font notice. Complex shaping/bidi
and LVGL (`lib/lvgl_port`) are App-owned; the OS has no LVGL.

## Built-in metadata and package selection

Factory metadata is a shared public contract recorded through SDK getters,
including all22parameters across five engines and Auto Gate. Preview keeps
the selected Gate variant and reports the same names, defaults, ranges, step
and flags as native metadata. Parameter writes do not change default metadata.

The firmware resolves installed algorithm ID, optional version, then
optional full-RPK CRC. Empty IDs/malformed descriptors return ERR_ARG; missing
ID returns ERR_PACK_ID(-10), unavailable requested version ERR_PACK_VERSION(-11),
CRC mismatch ERR_PACK_CRC(-12), multiple matches ERR_PACK_AMBIGUOUS(-13), and
invalid stored packages ERR_PACK_FORMAT(-14), and a selected older/unsupported
code target ERR_PACK_TARGET(-15). Actual storage read/list failures
remain ERR_STORAGE(-4). Verified matches remain usable when unrelated entries
are unreadable; without a match, an incomplete scan reports its storage/format
error instead of claiming an ID is absent. No mutation/pinning/processor
activation occurs during selection. Failed creation leaves output handle0 and
the live graph intact. Memory checks follow selection; CPU enforcement occurs during real audio.

Use resona_error_string(rc) for a short description or switch on the constants
for App-local translations. Preview does not load installed packs or simulate
built-in DSP; valid pack creation remains unsupported there.

Stored older targets remain inspectable for ID/version diagnostics but are
never executed or migrated. Only compatible matches participate in ambiguity
resolution. Pinning a full-RPK CRC selects one of multiple compatible versions/
files without replacing shared stored entries.

## App memory banks

The platform table has `memory_reserve`, `memory_alloc` and `memory_info`.

`LARGE` keeps the existing `heap_bytes` budget and `alloc(bytes, align)` behavior.
An App explicitly reserves a budget before allocating `FAST` or `REALTIME`.
FAST is suitable for ordinary processing work; the smaller REALTIME bank is for
latency-sensitive data. A request stays in the selected bank or fails. There is
no hidden fallback, fixed slot partition, or permanent unused factory reserve.
Release engines the App does not need before claiming their free memory.

`memory_reserve(bank, bytes)` reserves one owned arena. FAST rounds the physical
span to a power of two; REALTIME rounds to128 bytes. `memory_info` distinguishes
the allocation budget, bytes in use, physical span, and unowned device memory.
Available/largest-free are snapshots, before allocation alignment, not promises.
Largest-free is a nonblocking lower bound and may be zero while a heap is busy.
Reserved bytes describe the owned mapped span; global free memory additionally
charges allocator alignment padding and bookkeeping, which can be substantial.
An empty bank must be released with a zero budget before changing its size;
resizing/releasing a bank with live allocations returns `ERR_STATE`. `free`
accepts allocations from every bank. All banks share the64-record limit and
maximum4096-byte power-of-two alignment. Zero bytes, invalid banks/alignments,
and exhausted quotas return recoverable errors or NULL. Foreign frees and
access after arena release remain fatal memory-protection violations.

An App can reach only its own memory. Its state, large heap, framebuffer and stack are
reserved together and rounded up, so an App may take more memory than the sum of their
sizes; the extra belongs to that App.
Banks are released after an App fault and at reboot/unload. Platform reserves,
factory private memory, graph admission and audio deadline checks still apply.

The desktop preview validates bank ownership, budgets, alignment, record limits,
and release/reuse. It reports zero for unowned physical capacities because it
cannot predict live board memory or reproduce device memory-protection faults.
It does not measure bank speed or device CPU usage.

## Preset-program backend

Algorithms use module ABI 2: META has a 32-bit count, 4-byte alignment and 44-byte
parameter records. The descriptor's function offsets and size stay unchanged. Count is
bounded by available package/memory/storage and callback budgets; twelve is the delivery
batch width, not the number of declared controls. The visual patch editor still uses its
12-control layout.

A preset is the factory chain (Gate, Comp, Resona, NAM, Cab, each once, any order) or one
RPK on its own. Apps use `program.h`: `program_info`, `program_chain` (per effect source,
name, on/off, parameter count, model), `program_effect_param_info/get/set`,
`program_effect_enable`, `program_effect_model`, `program_map_encoder`, and `program_edit`
jobs (`RESONA_CHAIN_SET`, `RESONA_CHAIN_MOVE`). Edits are checked for CPU and memory before
they play; a refusal leaves the playing preset unchanged. Program storage calls return
UNSUPPORTED in App host preview.

On firmware with V7 AMP support, the Resona effect's live parameter count and
indexes follow the loaded model. After a successful model selection, read
`program_chain` and enumerate the effect's controls again. A mapped parameter
is reported only while that control is available on the loaded model.

The device has 50 independent preset snapshots, shared exact RPK references and a
variable-length system state file. Saving needs free contiguous copy-on-write space.
Clearing a preset keeps its currently playing sound until a recall; referenced files remain
protected. Explicit desktop initialization/recovery discards test content only on the
reviewed unit. Committed recalls refresh the Factory App's live preset and target. Trusted
preview/load/save work is credited separately from App user-code time.

