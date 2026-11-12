# Port the Deluge layers to v1.3.0

## Goal

Rebase the existing layer definitions onto official Deluge release `v1.3.0`, commit `156500e7c4b756840bd4b116e70a3b27c27d64bb`. Preserve intended fork behavior and upstream fixes, review overlapping functionality, and keep original provenance. [Owners](../../OWNERS.md).

## Scope and checks

- [x] Prepare an isolated candidate, pin the official release tag and commit, and preserve the current definition checkpoint.
- [x] Port the layers in order; reconcile conflicts and upstream overlaps rather than blindly choosing one side. Regenerate patch operations, and squash coherent fixups or follow-ups where doing so preserves behavior, dependencies, messages, and source provenance.
- [x] Review catalog/source references and update the README base metadata and relevant provenance for the new base.
- [x] Rebuild the complete stack from the actual definitions, run `layerctl check` and relevant host tests and firmware build where available, and review the resulting behavior and diff.
- [x] Integrate verified definitions locally and checkpoint. Do not push, install, or flash without further direction.

## Starting evidence

The [v1.3.0 audit](../artifacts/v1.3.0-layer-audit.md) records the upstream release, partial overlaps, and trial conflicts. The prior definition checkpoint is `45aad76bc8454651df76572cb318c28845531755`. The temporary release trial and its build inputs were removed after verification.

The user also requested squashing layers where a follow-up or fix belongs with its originating change. Identify defensible combinations, update affected dependencies and provenance, and verify the rebuilt stack.

## Result and validation

- Pinned official tag `v1.3.0` at `156500e7c4b756840bd4b116e70a3b27c27d64bb`; retained the original source metadata in `provenance.json`.
- Reconciled overlapping upstream changes and folded layers 0005, 0008, 0012, and 0020 into their related feature layers. The dependency graph now has 28 layers.
- Rebuilt the actual definitions into projection `firmware-v1-3-0`. Its tree `f86e4d4571ce39994b786abf528fa14f1642141d` matches the independently compiled candidate. `layerctl check`, Git connectivity, and all 27 host test suites passed.
- Built Release firmware with the existing v22 ARM toolchain. The resulting `deluge.bin` SHA-256 was `ce802aeb99711e71996a618c7e3e84151067f3d848f40482af8dbabc7688fbd8`; build outputs were removed after verification. The linker reported an RWX segment warning.
- Hardware behavior has not been tested. The reusable projection is linked as ignored `.layerctl-worktree` in the definitions checkout.
