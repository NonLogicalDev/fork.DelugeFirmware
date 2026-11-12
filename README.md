# Deluge Firmware layers

**Upstream base:** [v1.3.0](https://github.com/SynthstromAudible/DelugeFirmware/releases/tag/v1.3.0) · [156500e7](https://github.com/SynthstromAudible/DelugeFirmware/commit/156500e7c4b756840bd4b116e70a3b27c27d64bb) · November 3, 2026 (commit date)

This repository defines a customized Deluge Community Firmware using ordered [layerctl](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl) layers. The base layer imports the pinned upstream source, normalizes Korg directory casing, and adds a local Nix build environment. Other layers add firmware features and fixes.

## Feature highlights

- [Keyboard chords](DEVIATIONS.md#3-keyboard-view-exact-chord-recognition) and [the latest held note](DEVIATIONS.md#4-keyboard-view-chord-and-latest-physical-note-display) in Keyboard View.
- [Kit creation and reuse](DEVIATIONS.md#5-kit-sample-creation-entry-and-reuse), with improved [Manual Slicer controls](DEVIATIONS.md#6-manual-slicer-positioning-and-preview-stop).
- An emergency [Panic action](DEVIATIONS.md#9-panic-action) and [copy and paste for Kit rows](DEVIATIONS.md#10-copy-paste-and-reorder-a-kit-sound-drum-row).
- [MIDI sustain pedal support](DEVIATIONS.md#11-optional-incoming-midi-sustain-pedal) and [externally stepped MIDI clips](DEVIATIONS.md#23-externally-stepped-midi-clips).
- [OLED waveform displays](DEVIATIONS.md#17-oled-sample-editing-waveform-companions), a [clip progress ruler](DEVIATIONS.md#18-oled-clip-timeline-ruler), and [recoverable Audio Clip start editing](DEVIATIONS.md#20-recoverable-audio-clip-start-editing).
- [Velocity Drums profiles](DEVIATIONS.md#22-velocity-drums-velocity-profiles), [Kit choke groups](DEVIATIONS.md#27-kit-sound-drum-choke-groups), and [fine envelope time controls](DEVIATIONS.md#30-fine-envelope-time-controls).

See [CHANGELOG.md](CHANGELOG.md) for fork changes, [DEVIATIONS.md](DEVIATIONS.md) for detailed behavior, and the [keybinding catalog](keybindings/README.md) for the control reference.

## Create a firmware workspace

Install Perl and [layerctl](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl), configure your Git author and committer identity, and ensure SSH access to GitHub. From this repository, run:

```sh
layerctl projection create firmware --worktree /path/to/new-firmware-workspace
layerctl check
```

The workspace path must not already exist. The base clone prepares the casing fix in Git before writing files, so the workspace can be created on a case-insensitive filesystem.

## Build locally

The included Nix environment targets Apple Silicon macOS. From the firmware workspace, run:

```sh
nix develop path:./.nix
DBT_NO_SYNC=1 ./dbt build Release
```

The build produces `build/Release/deluge.bin`. The workspace’s `.nix/README.md` describes the toolchain, compiler cache, and build configuration.

## Work with layers

Each directory in `layers/` contains a `layer.toml` defining its dependencies, commit message, and operations. Layerctl applies enabled layers in numbered order. Use a conventional commit subject and wrap message bodies to a 90-character target.

To make a change, create a projection through the relevant layer, commit the edit in that workspace, and use `layerctl layer add` or `layerctl layer refresh`. The [layerctl authoring walkthrough](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl/skills/layerctl) covers editing, refresh, conflict recovery, and cleanup. `provenance.json` records source commit metadata, and [project plans](docs/plans/README.md) track repository work.

## Layer graph

Arrows point from prerequisites to dependent layers; colors identify the main firmware area each layer changes. After changing dependencies, regenerate the diagram with `layerctl graph --format mermaid | python3 scripts/graph_layers_color.py --write`. Assign an area in the script when adding a layer.

```mermaid
flowchart LR
  n0["0001-base"]
  n1["0002-add-keyboard-view-chord-detection"]
  n2["0003-add-manual-slicer-kit-creation-option"]
  n3["0004-improve-manual-slicer-controls"]
  n4["0005-add-panic-action"]
  n5["0006-copy-and-paste-kit-rows"]
  n6["0007-reuse-a-kit-from-manual-slice"]
  n7["0008-make-kit-row-reordering-undoable"]
  n8["0009-add-midi-sustain-pedal-feature-flag"]
  n9["0010-add-song-grid-track-column-glow"]
  n10["0011-fix-repeated-audio-looper-recording"]
  n11["0012-synchronize-grid-and-audio-clip-colors"]
  n12["0013-keep-waveform-rendering-valid-for-long-samples"]
  n13["0014-add-efficient-oled-waveform-displays"]
  n14["0015-add-compact-oled-clip-progress-ruler"]
  n15["0016-audition-current-waveform-editor-kit-row"]
  n16["0017-add-recoverable-audio-clip-start-editing"]
  n17["0018-improve-waveform-editor-entry-and-bound-selection"]
  n18["0019-add-velocity-drums-velocity-profiles"]
  n19["0020-reject-unsupported-song-compatibility-versions"]
  n20["0021-add-external-step-midi-clips"]
  n21["0022-make-preview-only-note-input-release-order-safe"]
  n22["0023-correct-slicer-playhead-source"]
  n23["0024-improve-waveform-playhead-visibility"]
  n24["0025-add-kit-choke-groups"]
  n25["0026-add-shared-song-scale-menu"]
  n26["0027-add-precise-envelope-time-controls"]
  n0 --> n1
  n0 --> n2
  n2 --> n3
  n0 --> n4
  n0 --> n5
  n3 --> n6
  n2 --> n6
  n0 --> n7
  n0 --> n8
  n0 --> n9
  n0 --> n10
  n6 --> n11
  n10 --> n11
  n0 --> n12
  n6 --> n13
  n12 --> n13
  n5 --> n14
  n0 --> n15
  n7 --> n16
  n13 --> n16
  n14 --> n16
  n15 --> n17
  n14 --> n18
  n0 --> n19
  n6 --> n20
  n8 --> n20
  n19 --> n20
  n1 --> n21
  n18 --> n21
  n13 --> n22
  n16 --> n23
  n17 --> n23
  n22 --> n23
  n5 --> n24
  n11 --> n24
  n15 --> n24
  n20 --> n24
  n4 --> n25
  n9 --> n25
  n21 --> n25
  n23 --> n25
  n24 --> n25
  n25 --> n26
  subgraph legend["Firmware areas"]
    legend_foundation["Foundation"]
    legend_keyboard["Keyboard and scales"]
    legend_slicer["Slicer and Kit"]
    legend_waveform["Audio and waveforms"]
    legend_song["Song and arrangement"]
    legend_control["Playback and MIDI"]
    legend_sound["Sound and expression"]
  end
  classDef foundation fill:#e5e7eb,stroke:#4b5563,color:#111827
  class n0,n19,legend_foundation foundation
  classDef keyboard fill:#ede9fe,stroke:#6d28d9,color:#111827
  class n1,n21,n25,legend_keyboard keyboard
  classDef slicer fill:#ffedd5,stroke:#c2410c,color:#111827
  class n2,n3,n5,n6,n7,n24,legend_slicer slicer
  classDef waveform fill:#ccfbf1,stroke:#0f766e,color:#111827
  class n10,n12,n13,n14,n15,n16,n17,n22,n23,legend_waveform waveform
  classDef song fill:#dbeafe,stroke:#1d4ed8,color:#111827
  class n9,n11,legend_song song
  classDef control fill:#dcfce7,stroke:#15803d,color:#111827
  class n4,n8,n20,legend_control control
  classDef sound fill:#fce7f3,stroke:#be185d,color:#111827
  class n18,n26,legend_sound sound
```

## License

The firmware patches and included [LICENSE](LICENSE) retain their source license and attribution recorded in `provenance.json`.
