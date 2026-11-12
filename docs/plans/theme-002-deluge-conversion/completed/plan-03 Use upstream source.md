# Use the upstream Deluge repository as the base

## Goal

Use `https://github.com/SynthstromAudible/DelugeFirmware` as the base `git-clone` source for the converted Deluge layers. Preserve the fork's changes and provenance, and keep local work unpublished. [Owners](../../OWNERS.md).

## Work

- [x] Check the upstream refs and whether they contain the currently pinned base commit.
- [x] Change the base source and adjust layer definitions only as needed to preserve the intended firmware.
- [x] Verify the refactored layerctl CLI against upstream and preserve a local checkpoint.
- [x] Replace the casing patch with a pre-checkout script and verify the full projection and replay.

## Current state

The definition worktree is on `layerctl-conversion`. The base fetches upstream `main` at the original pinned commit and normalizes casing with a pre-checkout script.

- User requested all Deluge plans in the Deluge definition repository under `docs/plans`. Move the existing conversion theme and its history there, preserving the owner identity and worktree recovery records; repair links and verify both plan roots before continuing.
- Verified the original pinned base commit is reachable from upstream `main`; it is not reachable from upstream `community`. Use the upstream SSH URL and `refs/heads/main`, retaining the pinned commit.

- The user requested the official upstream repository. `9e3132dc17070695b40f69bada5565a33ce428b4` is an ancestor of upstream `refs/heads/main`, so update the clone URL to `git@github.com:SynthstromAudible/DelugeFirmware.git`, select that ref, retain the exact commit and casing preparation, and verify the existing resulting tree. Omit `dir` once layerctl supports the root default. Preserve the original fork URL and commit metadata in provenance.

- Initial full SSH projection from the official upstream `main` at the original pinned commit reproduced the recorded firmware tree and original base parent on a case-insensitive filesystem. The current layerctl operation reorganization is undergoing a second full SSH verification. Original fork source metadata remains in `provenance.json`; `conversion.base_source` and `conversion.base_ref` record the new upstream source.

- User requested that `0001-base` use the new pre-checkout script. Replace the casing patch with a layer-local executable that rewrites the KORG directory paths in the prepared Git index and prints the resulting tree SHA. Preserve the normalized base and final firmware trees, remove the superseded patch after verifying its contents remain recoverable in Git, and document any script prerequisites.

- The final executable script returns the normalized index tree, preserving file modes and blob IDs. The script rejects destination collisions. The former patch remains recoverable from Git history. Python 3 is documented as a prerequisite.
- A full SSH rebuild against official upstream reproduced the recorded firmware tree. After the final collision check was added, a full local-source projection and `layerctl check` replay on a case-insensitive filesystem reproduced final tree `e184389511a711f129d12b6064c2d29f760babaf` and base parent `9e3132dc17070695b40f69bada5565a33ce428b4`. The original fork source and provenance remain intact.
