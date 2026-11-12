---
description: "Active plan to render the Slicer grid immediately when an entry mode opens."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Refresh Slicer view on entry

## Purpose

Show the Slicer grid as soon as Manual slice opens, without requiring an unrelated Horizontal Encoder movement. This plan began on [[2026-08-22]].

## Scope

- In scope: request the existing Slicer UI redraw when it receives focus so direct Manual slice entry immediately shows its Manual Slicer grid and waveform.
- Keep stable: Manual Slice mode selection, ordinary and Shift Horizontal Encoder behavior, existing Region Slice entry, waveform data, preview behavior, slicing, saving, MIDI, and hardware installation behavior.
- Out of scope: new rendering architecture, waveform algorithms, control mappings, preview-stop changes owned by P15, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [ ] Choosing Manual slice shows the Manual Slicer grid and waveform without a Horizontal Encoder movement or other extra input.
- [ ] Existing Region Slice entry also redraws correctly on entry.
- [ ] The initial redraw does not change a slice position, mode, sample, selected file, or audio state.
- [x] `DEVIATIONS.md` includes the entry-render guarantee as part of the Manual Slice entry contract.
- [x] The source change is folded into `nl/patch-sample-slicer-01`, not added as a later cancellation or duplicate sample-slicer patch.
- [ ] Host unit tests and a local Nix Release build pass. No flashing or publication occurs.

## Current State

- Status: source implementation complete; runtime validation remains.
- Findings: `Slicer::focusRegained()` establishes `slicerMode`, initializes the manual state, and redraws only seven-segment text. It does not request a pad-grid render. `horizontalEncoderAction()` does request a full render, matching the reported need to jiggle that encoder before the Manual Slicer view appears.
- Decision: add the smallest entry-time request for the existing full UI rendering path. Apply it to Slicer focus rather than special-casing the Manual Slice menu option, so both Manual and Region entry paths remain coherent.
- Implementation: Slicer focus now requests the existing full UI render after it establishes the initial mode and manual state. The change is folded into `nl/patch-sample-slicer-01` at `12adadea91a4`; the later sample-slicer and Panic revisions rebased cleanly.
- Worker: `/root` completed source implementation, revision folding, and source review; runtime validation remains.
- Revision: this corrects P10's direct Manual Slice entry. `nl/patch-sample-slicer-04` was not created.
- Validation: the requested local Nix Release build completed on [[2026-08-22]] with `DBT_NO_SYNC=1` and produced the normal Release target. Its CMake version metadata read repository state as part of the build; no direct Git command, flashing, installation, publishing, or upstream submission occurred. Host unit tests and physical-device confirmation remain.
- Next action: confirm that Manual slice and standard Slice show their grids immediately on the physical device, then run host unit tests if further source work requires them. Do not flash or publish.
