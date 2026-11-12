# Deluge Firmware layers

This repository defines a Git-backed layerctl composition of the Deluge Firmware fork. `layers/0001-base` imports the official upstream `main` base at a pinned commit, runs [a casing-normalization script](layers/0001-base/scripts/git_normalize_casing.pl) against Git's index before checking out files, and adds the local Nix build environment. The following layers reproduce the remaining firmware changes in order. Their commit messages use conventional subjects and explain the changes and their purpose; the original messages remain in `provenance.json`. The layer directories are ordered by their four-digit prefixes.

`provenance.json` records the original source commits, trees, authors, commit headers including JJ change IDs, messages, and remote branch tips. The empty original change at `711d48340ddb9c42d65db6088434b4b9c2ba5e58` remains in provenance but has no layer. The original branches and history remain available at the source remote. Layerctl creates new workspace commits, so those commit IDs and JJ headers differ from the originals. The original casing-fix and Nix environment layers are recorded in provenance as integrated into the base layer.

[Project plans](docs/plans/README.md) include the conversion history and current work.

[DEVIATIONS.md](DEVIATIONS.md) preserves the fork’s feature and compatibility notes outside the composed firmware source. It is copied from the original tip without edits; paths in that document refer to the firmware workspace, and its maintenance instructions describe the original source layout. The original firmware README link to that file is omitted from the composition.

## Layer graph

Arrows point from prerequisites to dependent layers. The manifests declare both feature prerequisites and dependencies needed for patches to apply. Regenerate this diagram with `layerctl graph --format mermaid` after changing dependencies.

```mermaid
flowchart LR
  n0["0001-base"]
  n1["0004-add-keyboard-view-chord-detection"]
  n2["0005-show-chord-and-latest-pressed-note"]
  n3["0006-add-manual-slicer-kit-creation-option"]
  n4["0007-improve-manual-slicer-controls"]
  n5["0008-stop-preview-when-opening-manual-slice"]
  n6["0009-add-panic-action"]
  n7["0010-copy-and-paste-kit-rows"]
  n8["0011-reuse-a-kit-from-manual-slice"]
  n9["0012-add-shift-five-tap-panic-gesture"]
  n10["0013-make-kit-row-reordering-undoable"]
  n11["0015-add-midi-sustain-pedal-feature-flag"]
  n12["0016-add-song-grid-track-column-glow"]
  n13["0017-fix-repeated-audio-looper-recording"]
  n14["0018-synchronize-grid-and-audio-clip-colors"]
  n15["0019-keep-waveform-rendering-valid-for-long-samples"]
  n16["0020-correct-reversed-waveform-row-ranges"]
  n17["0021-add-efficient-oled-waveform-displays"]
  n18["0022-add-compact-oled-clip-progress-ruler"]
  n19["0023-audition-current-waveform-editor-kit-row"]
  n20["0024-add-recoverable-audio-clip-start-editing"]
  n21["0025-improve-waveform-editor-entry-and-bound-selection"]
  n22["0026-add-velocity-drums-velocity-profiles"]
  n23["0027-reject-unsupported-song-compatibility-versions"]
  n24["0028-add-external-step-midi-clips"]
  n25["0029-make-preview-only-note-input-release-order-safe"]
  n26["0030-correct-slicer-playhead-source"]
  n27["0031-improve-waveform-playhead-visibility"]
  n28["0032-add-kit-choke-groups"]
  n29["0033-catalog-firmware-keybindings"]
  n30["0034-add-shared-song-scale-menu"]
  n31["0035-add-precise-envelope-time-controls"]
  n0 --> n1
  n1 --> n2
  n0 --> n3
  n3 --> n4
  n3 --> n5
  n0 --> n6
  n0 --> n7
  n4 --> n8
  n5 --> n8
  n6 --> n9
  n0 --> n10
  n0 --> n11
  n0 --> n12
  n0 --> n13
  n8 --> n14
  n13 --> n14
  n0 --> n15
  n15 --> n16
  n8 --> n17
  n15 --> n17
  n7 --> n18
  n0 --> n19
  n10 --> n20
  n17 --> n20
  n18 --> n20
  n19 --> n21
  n18 --> n22
  n0 --> n23
  n8 --> n24
  n11 --> n24
  n23 --> n24
  n2 --> n25
  n22 --> n25
  n17 --> n26
  n20 --> n27
  n21 --> n27
  n26 --> n27
  n7 --> n28
  n14 --> n28
  n19 --> n28
  n24 --> n28
  n9 --> n29
  n12 --> n29
  n25 --> n29
  n27 --> n29
  n28 --> n29
  n29 --> n30
  n30 --> n31
```

## Rebuild

Install Perl and `layerctl` from [misc.experiments/utils/layerctl](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl), configure Git author and committer identity, and ensure SSH access to the upstream repository. Then run from this definition repository:

```sh
layerctl graph --format mermaid
layerctl projection create firmware --worktree /path/to/new-firmware-workspace
layerctl check
```

The workspace path must not already exist. The base clone prepares the casing fix before files are written, so it can run on a case-insensitive filesystem. The resulting firmware tree omits `DEVIATIONS.md` and the original README link to it; those intentional differences mean its tree ID differs from the original recorded in `provenance.json`. For the initial converted stack, compare `git -C /path/to/new-firmware-workspace rev-parse HEAD^{tree}` with `conversion.expected_initial_tree` in `provenance.json`.

For changes, create a separate projection through the relevant layer, commit the edit there, and use `layerctl layer add` or `layerctl layer refresh` from this repository. Write each layer commit message with a conventional commit subject and a body that explains what changed and why it matters. Wrap body text to a 90-character target. The [layerctl skill walkthrough](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl/skills/layerctl) covers editing, refresh, conflict recovery, and cleanup. Preserve `provenance.json` as the record of the initial conversion when later changes are made.

The included LICENSE is copied from the original firmware fork. The patches are derived from that project and retain its license and original attribution recorded in `provenance.json`.
