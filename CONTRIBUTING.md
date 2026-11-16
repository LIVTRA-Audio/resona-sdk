# Contributing

This is the public SDK source tree, licensed under GPL-3.0. Every change must work with
only the files in this repository. Use relative documentation links and product-facing
Resona API names. Keep existing ABI numbers, numeric wire identifiers and table layouts
compatible; coordinate runtime changes separately.

## What belongs here

- Only public SDK code, examples, guides and the user-program runtime. Never platform
  firmware, factory DSP, trained models, drivers, firmware images, credentials or
  signing material.
- Third-party code only under a GPL-3.0-compatible license; add its required notice to
  `THIRD_PARTY_NOTICES.md`.
- Contributions are accepted under GPL-3.0 and Livtra's contributor license agreement
  (CLA), which lets Livtra also use them in its own products. Sign it before your first
  pull request is merged.
- Public factory UI artwork and glyph tables are intentional source assets.
  Keep `apps/resona_factory/src/generated/`: those `.inc` arrays are compiled
  UI source data, not compiler outputs.

## Local verification

Run from this folder:

```sh
python3 tools/check_source.py
```

On Windows, `py -3 tools/check_source.py` is equivalent. The checker is offline,
read-only and uses only Python's standard library. It checks for generated output,
common sensitive file types/private-key markers, the RBX1 target definition,
local Markdown links and example/template source references. For the chip-name
audit, add `--terms-file /path/to/private-policy.txt`: one forbidden term per line,
with blank lines and `#` comments ignored. Keep that policy outside this SDK and
outside any public distribution. The default check explicitly reports when no
name policy was supplied. It cannot prove that all
secrets or third-party licensing issues have been found; review new files too.

Build examples in copies outside this tree using
[Getting started](docs/getting-started.md). Record the actual host OS, Creator Lab,
compiler and firmware used. Host checks cannot establish hardware performance or
Windows compatibility. Do not install test fixtures or flash firmware as an
automatic part of source checks.

## Before a separate release

1. Run the source checker on the exact folder to be distributed.
2. Check notices and review every newly added asset or dependency.
3. Build an algorithm and Box App from this folder alone.
4. Document the tested Lab/firmware versions and platform coverage.
5. Assign a release version and release notes when a release is authorized.

Initializing Git, creating a remote, tagging and publishing are separate actions.
