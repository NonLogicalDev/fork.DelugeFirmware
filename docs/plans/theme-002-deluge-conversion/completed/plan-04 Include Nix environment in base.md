# Include the Nix environment in the base layer

## Goal

Move the local Nix build environment from `0003-add-local-nix-build-environment` into `0001-base`, retain its original provenance, and preserve the final composed firmware tree. [Owners](../../OWNERS.md).

## Work

- [x] Add the Nix files as a base layer overlay after its clone operation, preserving their contents and describing the environment in the base commit message.
- [x] Record the original Nix layer as integrated in provenance, remove its old layer directory, and regenerate the dependency graph.
- [x] Replay the full composition and `layerctl check`, confirm the original final tree and base-parent ancestry, and checkpoint the changes locally.

The three Nix files were verified against the original patch’s Git blob IDs. A case-insensitive full projection and `layerctl check` reproduced original Nix layer tree `3571b14f010533c4d07d32e7c180814711e7d87e` as the base tree, retained upstream parent `9e3132dc17070695b40f69bada5565a33ce428b4`, and reproduced final tree `e184389511a711f129d12b6064c2d29f760babaf`. The generated Mermaid graph matches the README.
