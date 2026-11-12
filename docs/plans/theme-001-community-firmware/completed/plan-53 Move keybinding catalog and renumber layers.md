# Move the keybinding catalog and renumber layers

Move the catalog into the definitions repository, remove its firmware layer, preserve its later updates, and renumber all remaining layers without changing firmware behavior. Preserve source provenance and the existing firmware binary.

- [x] Update catalog source paths and repository links.
- [x] Renumber layers and repair dependencies, provenance, and graph.
- [x] Rebuild the stack and compare its firmware tree with the previous projection, excluding the relocated catalog.
- [x] Run catalog checks and record existing source drift.

## Result

- Preserved all catalog records, including changes from the two later firmware layers, under `keybindings/data/`; moved maintenance tools to `keybindings/scripts/` and kept generated reports ignored.
- Removed the catalog layer and renumbered the 27 remaining layers. Updated dependencies, provenance, graph, and source-check paths.
- A fresh projection reproduced every previous firmware file byte-for-byte except the 12 relocated catalog files. All 13 catalog tests and validation of 1,275 records passed. The audit output was identical before and after the move: 68 changed source files and three unreviewed handler candidates remain from the v1.3.0 port.
- Preserved the earlier `.layerctl-worktree` projection and its firmware binary; it predates the layer renumbering and retains the historical catalog files. New projections use the new definitions.
