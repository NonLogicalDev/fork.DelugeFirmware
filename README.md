# Deluge Firmware layers

This repository defines a Git-backed layerctl composition of the Deluge Firmware fork. `layers/0001-base` imports the official upstream `main` base at a pinned commit and runs [a casing-normalization script](layers/0001-base/scripts/git_normalize_casing.py) against Git's index before checking out files. The following layers reproduce the remaining firmware changes in order. Their commit messages explain the changes and their purpose; the original messages remain in `provenance.json`. The layer directories are ordered by their four-digit prefixes.

`provenance.json` records the original source commits, trees, authors, commit headers including JJ change IDs, messages, and remote branch tips. The empty original change at `711d48340ddb9c42d65db6088434b4b9c2ba5e58` is retained as a no-op marker with a descriptive layer commit message; its original message was empty. The original branches and history remain available at the source remote. Layerctl creates new workspace commits, so those commit IDs and JJ headers differ from the originals. The original casing-fix layer is recorded in provenance as integrated into the base clone.

[Project plans](docs/plans/README.md) include the conversion history and current work.

[DEVIATIONS.md](DEVIATIONS.md) preserves the fork’s feature and compatibility notes outside the composed firmware source. It is copied from the original tip without edits; paths in that document refer to the firmware workspace, and its maintenance instructions describe the original source layout. The original firmware README link to that file is omitted from the composition.

## Layer graph

Arrows point from prerequisites to dependent layers. The manifests declare both feature prerequisites and dependencies needed for patches to apply. Regenerate this diagram with `layerctl graph --format mermaid` after changing dependencies.

```mermaid
flowchart LR
  n0["0001-base"]
  n1["0003-add-local-nix-build-environment"]
  n2["0004-add-keyboard-view-chord-detection"]
  n3["0005-show-chord-and-latest-pressed-note"]
  n4["0006-add-manual-slicer-kit-creation-option"]
  n5["0007-improve-manual-slicer-controls"]
  n6["0008-stop-preview-when-opening-manual-slice"]
  n7["0009-add-panic-action"]
  n8["0010-copy-and-paste-kit-rows"]
  n9["0011-reuse-a-kit-from-manual-slice"]
  n10["0012-add-shift-five-tap-panic-gesture"]
  n11["0013-make-kit-row-reordering-undoable"]
  n12["0014-empty-change-marker"]
  n13["0015-add-midi-sustain-pedal-feature-flag"]
  n14["0016-add-song-grid-track-column-glow"]
  n15["0017-fix-repeated-audio-looper-recording"]
  n16["0018-synchronize-grid-and-audio-clip-colors"]
  n17["0019-keep-waveform-rendering-valid-for-long-samples"]
  n18["0020-correct-reversed-waveform-row-ranges"]
  n19["0021-add-efficient-oled-waveform-displays"]
  n20["0022-add-compact-oled-clip-progress-ruler"]
  n21["0023-audition-current-waveform-editor-kit-row"]
  n22["0024-add-recoverable-audio-clip-start-editing"]
  n23["0025-improve-waveform-editor-entry-and-bound-selection"]
  n24["0026-add-velocity-drums-velocity-profiles"]
  n25["0027-reject-unsupported-song-compatibility-versions"]
  n26["0028-add-external-step-midi-clips"]
  n27["0029-make-preview-only-note-input-release-order-safe"]
  n28["0030-correct-slicer-playhead-source"]
  n29["0031-improve-waveform-playhead-visibility"]
  n30["0032-add-kit-choke-groups"]
  n31["0033-catalog-firmware-keybindings"]
  n32["0034-add-shared-song-scale-menu"]
  n33["0035-add-precise-envelope-time-controls"]
  n0 --> n1
  n0 --> n2
  n2 --> n3
  n0 --> n4
  n4 --> n5
  n4 --> n6
  n0 --> n7
  n0 --> n8
  n5 --> n9
  n6 --> n9
  n7 --> n10
  n0 --> n11
  n0 --> n12
  n0 --> n13
  n0 --> n14
  n0 --> n15
  n9 --> n16
  n15 --> n16
  n0 --> n17
  n17 --> n18
  n9 --> n19
  n17 --> n19
  n8 --> n20
  n0 --> n21
  n11 --> n22
  n19 --> n22
  n20 --> n22
  n21 --> n23
  n20 --> n24
  n0 --> n25
  n9 --> n26
  n13 --> n26
  n25 --> n26
  n3 --> n27
  n24 --> n27
  n19 --> n28
  n22 --> n29
  n23 --> n29
  n28 --> n29
  n8 --> n30
  n16 --> n30
  n21 --> n30
  n26 --> n30
  n10 --> n31
  n14 --> n31
  n27 --> n31
  n29 --> n31
  n30 --> n31
  n31 --> n32
  n32 --> n33
```

## Rebuild

Install Python 3 and install `layerctl` from [misc.experiments/utils/layerctl](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl), configure Git author and committer identity, and ensure SSH access to the upstream repository. Then run from this definition repository:

```sh
layerctl graph --format mermaid
layerctl projection create firmware --worktree /path/to/new-firmware-workspace
layerctl check
```

The workspace path must not already exist. The base clone prepares the casing fix before files are written, so it can run on a case-insensitive filesystem. The resulting firmware tree omits `DEVIATIONS.md` and the original README link to it; those intentional differences mean its tree ID differs from the original recorded in `provenance.json`. For the initial converted stack, compare `git -C /path/to/new-firmware-workspace rev-parse HEAD^{tree}` with `conversion.expected_initial_tree` in `provenance.json`.

For changes, create a separate projection through the relevant layer, commit the edit there, and use `layerctl layer add` or `layerctl layer refresh` from this repository. Write each layer commit message with a subject and body that explain what changed and why it matters. Keep commit-message lines at most 120 characters. The [layerctl skill walkthrough](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl/skills/layerctl) covers editing, refresh, conflict recovery, and cleanup. Preserve `provenance.json` as the record of the initial conversion when later changes are made.

The included LICENSE is copied from the original firmware fork. The patches are derived from that project and retain its license and original attribution recorded in `provenance.json`.
