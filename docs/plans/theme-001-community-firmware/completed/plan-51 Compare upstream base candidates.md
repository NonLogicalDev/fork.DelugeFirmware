# Compare upstream base candidates

## Goal

Audit every existing layer against official release `v1.3.0`, identify equivalent or overlapping upstream behavior, and compare the release with current `main` as possible bases. Recommend a candidate using verified source, ancestry, and isolated layer replay. [Owners](../../OWNERS.md).

## Scope

- [x] Identify current official branch and stable release refs and compare their ancestry with the pinned base.
- [x] Audit all layers against the release source and upstream changes, distinguishing equivalent, partial, absent, and uncertain behavior. Replay the layers in temporary workspaces and record conflicts.
- [x] Recommend a candidate and preserve the existing base and published branches until the user chooses the intended direction.

Upstream `main` is `96c0028037f5945c9dd9ec55c7154c1489ce0681`; release `v1.3.0` is `156500e7c4b756840bd4b116e70a3b27c27d64bb`. Both descend from current base `9e3132dc17070695b40f69bada5565a33ce428b4`, but release and main diverged. Initial full replays reached `0009-add-panic-action`; the release has a reverb conflict, while main has conflicts in reverb and audio engine. Audit remaining layers independently.

The [layer-by-layer v1.3.0 audit](../artifacts/v1.3.0-layer-audit.md) covers all 32 layer definitions and links to release source evidence. No full layer is confirmed redundant; reverb pointer initialization, looper fixes, waveform byte widening, the OLED generation counter, compatibility checks, and trim-from-start have partial overlaps. The trial required provisional conflict resolutions and stopped at the waveform renderer; no port, firmware build, or hardware check was performed. Recommend the stable tag as the candidate, subject to the user’s direction.
