# Public SDK source instructions

This folder is intended to stand alone as a GPL-3.0 source distribution. Read
`README.md`, `CONTRIBUTING.md` and `agent/docs/contracts.md` before changing contracts.

- Preserve public ABI/wire compatibility. Document API changes and required
  runtime support; never imply that updating headers updates a physical device.
- Use Resona product abstractions. Keep silicon vendor/model/core information,
  firmware, factory DSP code, models and credentials out of this tree.
- Keep licensing and provenance with sources. No compiler or Qt binary belongs
  in this SDK distribution.
- Test in copied user projects outside this tree. Keep compiler output out of
  the SDK; run `python3 tools/check_source.py` before handing off.
- For a chip-name audit, supply `--terms-file` pointing outside the distribution.
  Do not put forbidden-name lists in public checkers, including encoded strings.
- Check host and device behavior separately; record only what was actually tested.
- Hardware writes, repository creation and publishing require an explicit user
  request. Source checks do not perform those actions.

`agent/AGENTS.pack.md` and `agent/AGENTS.app.md` are templates for generated user
projects. Their rules about read-only pinned copies apply to those projects;
public SDK maintainers edit the original sources here.
