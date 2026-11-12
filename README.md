# Deluge Firmware layers

**Upstream base:** [9e3132dc](https://github.com/SynthstromAudible/DelugeFirmware/commit/9e3132dc17070695b40f69bada5565a33ce428b4) · September 13, 2026 · [1.3.0-dev](https://github.com/SynthstromAudible/DelugeFirmware/blob/9e3132dc17070695b40f69bada5565a33ce428b4/CMakeLists.txt#L6-L18) (default development version)

This repository defines a customized Deluge Community Firmware using ordered [layerctl](https://github.com/NonLogicalDev/misc.experiments/tree/main/utils/layerctl) layers. The base layer imports the pinned upstream source, normalizes Korg directory casing, and adds a local Nix build environment. Other layers add firmware features and fixes.

## Feature highlights

- [Keyboard chords](DEVIATIONS.md#3-keyboard-view-exact-chord-recognition) and [the latest held note](DEVIATIONS.md#4-keyboard-view-chord-and-latest-physical-note-display) in Keyboard View.
- [Kit creation and reuse](DEVIATIONS.md#5-kit-sample-creation-entry-and-reuse), with improved [Manual Slicer controls](DEVIATIONS.md#6-manual-slicer-positioning-and-preview-stop).
- An emergency [Panic action](DEVIATIONS.md#9-panic-action) and [copy and paste for Kit rows](DEVIATIONS.md#10-copy-paste-and-reorder-a-kit-sound-drum-row).
- [MIDI sustain pedal support](DEVIATIONS.md#11-optional-incoming-midi-sustain-pedal) and [externally stepped MIDI clips](DEVIATIONS.md#23-externally-stepped-midi-clips).
- [OLED waveform displays](DEVIATIONS.md#17-oled-sample-editing-waveform-companions), a [clip progress ruler](DEVIATIONS.md#18-oled-clip-timeline-ruler), and [recoverable Audio Clip start editing](DEVIATIONS.md#20-recoverable-audio-clip-start-editing).
- [Velocity Drums profiles](DEVIATIONS.md#22-velocity-drums-velocity-profiles), [Kit choke groups](DEVIATIONS.md#27-kit-sound-drum-choke-groups), and [fine envelope time controls](DEVIATIONS.md#30-fine-envelope-time-controls).

See [DEVIATIONS.md](DEVIATIONS.md) for the full feature and compatibility notes.

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

Arrows point from prerequisites to dependent layers. Regenerate the diagram with `layerctl graph --format mermaid` after changing dependencies.

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

## License

The firmware patches and included [LICENSE](LICENSE) retain their source license and attribution recorded in `provenance.json`.
