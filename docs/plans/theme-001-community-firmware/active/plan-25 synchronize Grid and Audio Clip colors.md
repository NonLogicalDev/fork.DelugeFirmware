---
description: "Active plan to make each Audio Clip waveform use the pastel form of its Song Grid track color."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Synchronize Grid and Audio Clip colors

## Purpose

Make track color consistent between Song Grid and Audio Clip waveforms. The Grid track color is the source of truth: every Audio Clip on that track uses its pastel form, new clips inherit it immediately, and later Grid color changes update the waveform without maintaining a second color. This plan began on [[2026-08-24]].

## Product integration

- Existing model: Song Grid renders a track-level `Output::colour`, while each Audio Clip waveform renders a separate clip-level `colourOffset`. Those values are assigned and edited independently, so the Grid cell and waveform can disagree.
- User intent: the Grid column chooses the track's identity color. Audio Clip waveforms should present the same hue in the existing pastel waveform style.
- Integrated model: the Audio Output's track color is authoritative. An Audio Clip derives its waveform color from its current Output, so creation, reassignment, overdubbing, Grid editing, and save/reload cannot leave duplicate color state out of sync.
- Existing control: Audio Clip View's Shift + Vertical color gesture should edit the same track color rather than a clip-only value that would immediately diverge.
- Architecture impact: centralize Audio Clip waveform hue selection on its owning Audio Output and preserve the existing waveform amplitude rendering, Grid state brightness, serialized project compatibility, and non-audio color systems.

## Scope

### In scope

- Make every attached Audio Clip render the pastel form of its owning Audio Output's Grid hue.
- Give a new Audio Output a usable Grid color before its Audio Clip waveform first renders.
- Make Grid track-color changes update every Audio Clip waveform on that Output without copying or synchronizing a second value.
- Keep Audio Clip View's existing color gesture useful by making it adjust the shared track color.
- Preserve the behavior for in-place overdubs and cloned overdubs that create a new Output.
- Update `DEVIATIONS.md`, owner guidance, focused tests where practical, the configured host suite, and the local Nix Release build.

### Out of scope

- Instrument Clip note colors, Kit-row colors, section colors, Arranger clip-instance colors, clip-state brightness, new settings, firmware flashing, publication, or upstream submission.
- Removing the serialized Audio Clip `colourOffset`; it remains readable and writable for project compatibility even when an attached Audio Clip no longer uses it as the waveform source.

## Completion Criteria

- [x] A new Audio Clip immediately uses the pastel form of its Grid track color.
- [x] Existing Audio Clips on one track use that track's current hue and update after the Grid color changes.
- [x] In-place overdubs retain the same track and waveform hue; an overdub that creates a new Output follows the new column's hue.
- [x] Audio Clip View's Shift + Vertical gesture changes the shared track color and remains visible in both Audio Clip View and Song Grid.
- [x] Waveform shape and amplitude brightness, Grid active/inactive/selected/recording feedback, non-audio colors, and saved-project compatibility remain unchanged in the reviewed source and local build.
- [x] The self-contained deviation contract, focused checks, configured host suite, and local Nix Release build pass in a separate `song-grid` revision. No flashing or publication occurs.
- [ ] A physical Deluge confirms the solid-to-pastel hue match, redraw timing, and encoder feel.

## Blockers

None. A physical Deluge will still be required to confirm the final LED hue match after local validation.

## Artifacts

- **A1:** Grid-owned Audio Clip waveform color.
	- **Type:** Firmware model and UI change.
	- **State:** Locally implemented, reviewed, tested, and built; physical confirmation pending.
	- **Contents:** One track-owned hue rendered directly for Grid and as a pastel for Audio Clip waveforms.
	- **Location:** `src/deluge/model/output.cpp`, `src/deluge/model/output_colour.cpp`, `src/deluge/model/clip/audio_clip.cpp`, `src/deluge/gui/views/audio_clip_view.cpp`, and `src/deluge/gui/views/session_view.cpp`.
	- **SCM surface:** Local bookmark `nl/patch-song-grid-02` at `3b2a38604c67`, above `ns/bug-audio-looper-01`.

- **A2:** Deviation contract.
	- **Type:** Self-contained behavior specification.
	- **State:** Complete as deviation 14.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.

## Workers

- `/root`: completed local scope, integration, review, validation, and Jujutsu revision management; physical confirmation remains.
- `/root/looper_core`: stopped after the dedicated validation worker covered the relevant paths; no separate report used.
- `/root/color_validation`: completed read-only worker for persistence compatibility, regression risks, and practical test seams.
- `/root/color_review`: completed adversarial review, found the encoder batching defect, and found no actionable issue after its repair.

## Decisions

- **Decision:** Make the track/Output color authoritative rather than copying it into each Audio Clip.
	- **Rationale:** Direct derivation satisfies creation and later edits without stale duplicate state.

- **Decision:** Render the authoritative hue through the existing pastel waveform conversion.
	- **Rationale:** The user asked for the Grid color's pastel version, and the waveform's current pastel visual treatment should remain intact.

- **Decision:** Keep the existing Audio Clip color gesture but redirect it to the shared track color.
	- **Rationale:** Leaving it clip-local would break synchronization; disabling it would remove an established control without need.

## Implementation Steps

1. [x] Trace every Output and Audio Clip color initialization, editing, cloning, persistence, and waveform rendering path.
2. [x] Create a fresh Jujutsu change in the `song-grid` group without using Git.
3. [x] Implement the smallest single-source color model and compatibility behavior.
4. [x] Add focused regression coverage or record any host-harness gap, then perform an adversarial source review.
5. [x] Update `DEVIATIONS.md`, run formatting, the configured host suite, and the local Nix Release build.
6. [x] Assign `nl/patch-song-grid-02`, update owner records, and leave physical validation pending without flashing.

## Work Log

- [x] [[2026-08-24]]@09:12 - Created the feature plan after the user clarified that new Audio Clips inherit their Grid track color and later Grid color edits update the waveform.
	- **Evidence:** Current source keeps Grid track hue in `Output::colour` and waveform hue in `AudioClip::colourOffset`; the user selected the Grid value as authoritative.
	- **Effect:** The accepted design is live track-owned derivation, not a one-time color copy.
	- **Next:** Complete the source and compatibility audit before changing firmware files.

- [x] [[2026-08-24]]@09:24 - Implemented the first integrated pass in a fresh Jujutsu change without using Git.
	- **Evidence:** Audio Clip waveforms now derive their pastel hue from the owning Output; Grid, Audio Clip View, and Row Song View edit that shared hue; the Output owns lazy nonzero assignment; the initial focused color-step tests passed 4/4.
	- **Effect:** New, cloned, loaded, and same-Output Audio Clips converge on one track-owned hue without a synchronization loop. Grid coarse stepping also cannot store the reserved zero sentinel.
	- **Next:** Complete adversarial review, fix any findings, then run the full host suite and Release build.

- [x] [[2026-08-24]]@09:36 - Closed the local review and validation cycle and assigned `nl/patch-song-grid-02` at `3b2a38604c67`.
	- **Evidence:** Adversarial review caught per-callback integer rounding in the Audio color gesture; a fixed three-hue-unit step per detent now makes accelerated and individual callbacks equivalent, with a focused regression test. Re-review found no actionable defect. Formatting passed, CTest passed 7/7, and the Release ELF linked at 09:33.
	- **Effect:** The local feature revision contains the implementation, focused arithmetic seam, self-contained deviation contract, and repository guidance. No firmware was flashed or published.
	- **Next:** Check solid-versus-pastel hue matching, sibling redraw timing, and encoder feel on the physical Deluge.

## Validation

- **Command or action:** Focused host checks for hue conversion and source ownership, where the existing harness can exercise the real types.
	- **Expected result:** One Output hue produces the expected pastel waveform hue, Grid edits are visible without copying state, and saved legacy `colourOffset` data remains accepted.
	- **Observed result:** The extracted production color-step seam passed 5 tests and 10 checks, including positive and negative wrap, the zero sentinel, accelerated steps, and encoder batching. Source review confirmed full-screen, Row, and animation waveforms all use `AudioClip::getColour()`; both Audio gestures and Grid edit the same Output. The lightweight host harness does not link the real Audio Clip, Output, or UI handlers, so the multi-clip rendering invariant remains a reviewed-source and hardware check rather than a direct unit test.
	- **Failure signals:** A waveform still depends on a separate clip color, an uninitialized track renders an unintended hue, or the change requires duplicate synchronization state.
	- **Interpretation:** The arithmetic and source ownership are locally covered in proportion to the available harness. Device rendering remains pending.

- **Command or action:** Configured CTest suite and local Nix Release build.
	- **Expected result:** All configured tests pass and the Release firmware links without a new warning attributable to this feature.
	- **Observed result:** `ctest --test-dir build/test-unit --output-on-failure` passed 7 of 7 targets. `ninja -C build -f build-Release.ninja -j 1 deluge` linked `build/Release/deluge.elf`, `.bin`, and `.hex`. Clang-format dry-run passed. The only warnings were the existing `view.cpp` fallthrough warning and the established RWX LOAD-segment linker warning; none referenced changed feature files.
	- **Failure signals:** Test failure, compiler error, link failure, or a new relevant warning.
	- **Interpretation:** Local compilation and regression validation passed.

- **Command or action:** Physical comparison of Grid column and Audio Clip waveform before and after color editing, including an overdub-created track.
	- **Expected result:** The waveform reads as the pastel version of the same hue and updates with the column.
	- **Observed result:** Pending user hardware test after the local build.
	- **Failure signals:** Different hue families, stale waveform color, or damaged clip-state brightness.
	- **Interpretation:** Only this device check proves the final LED appearance.

## Unfinished Work

- [ ] Confirm the final visual relationship on the physical Deluge before any flashing decision.
