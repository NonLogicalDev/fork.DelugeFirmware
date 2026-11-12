---
description: "Active plan to permit Manual Slice kit creation from a Kit's selected top Sound Drum."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Reuse a Kit from Manual Slice

## Purpose

Let Manual Slice reuse an existing Kit from a selected top Sound Drum, so a player can replace that row with a deliberate manual-sliced sample and append its remaining slices above it. This plan began on [[2026-08-22]].

## Product integration

- Existing model: Manual Slice kit creation can reuse an existing Kit only from a newly created empty Sound Drum at its safe top edge.
- User intent: add manually placed slices to a Kit that is otherwise usable, beginning at the selected top Sound Drum and growing upward from it. The route must work as a deliberate normal entry, not as a timing-sensitive shortcut, and Manual Slicer pads must audition the selected sample.
- Integrated model: the selected Sound Drum is the first destination. Eligibility looks only above it. The Manual Slice choice retains the selected file on that row before stopping the separate browser preview and opening the Slicer. The first created slice replaces the selected row and each later slice appends above it. Content above the selected row continues to block entry, so the operation cannot overwrite or shift a higher row.
- Architecture impact: widen the existing Manual Slice eligibility without changing its established direct-entry source retention, sample handling, confirmation, or cancellation paths.
- Existing behavior preserved: empty-Kit Manual Slice stays available; standard Slice, Load all, existing rows below the selected Sound Drum, Kit master settings, and all existing Manual Slicer editing controls stay unchanged.

## Scope

- In scope: a selected Sound Drum whose immediately higher and all later rows are unassigned may enter Manual Slice whether that selected row is empty or has a sample; its first generated slice replaces that row and additional slices are appended above it.
- Keep stable: a selected MIDI or Gate Drum, or a selected Sound Drum below existing Kit content, remains ineligible; existing rows and note data below the selected row, Kit settings, and source sample data outside the selected row are not replaced, moved, or cleared. Back keeps the established Manual Slicer exit behavior: the selected row remains as one full-sample pad. The reuse route stays in Manual mode so it cannot enter the separate Region Slice allocator.
- Out of scope: inserting below the selected row, reordering existing rows, replacing or moving a Kit drum other than the selected Sound Drum, changing slicing algorithms, undo/redo, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [x] An empty Kit can still enter Manual Slice exactly as before.
- [x] A selected topmost Sound Drum can enter Manual Slice whether it already has a sample or is newly empty. Its first generated slice is assigned to that pad and each later slice is assigned to a pad above it.
- [ ] Choosing Manual Slice from its normal deliberate menu entry reaches an interactive Manual Slicer without a timing-sensitive button sequence, and its slice pad auditions the selected sample.
- [x] A selected MIDI or Gate Drum, or a selected Sound Drum below existing Kit content, remains ineligible and leaves all contents unchanged.
- [x] Back from Manual Slice preserves the established full-sample anchor behavior without changing existing Kit rows.
- [x] Both display types give the existing or equivalent clear eligibility feedback.
- [x] `DEVIATIONS.md` records the behavior as a self-contained, implementation-independent contract.
- [x] Focused source review, host tests where the source seam permits, and a local Nix Release build pass. No flashing or publication occurs.

## Current State

- Status: active. The selected-Sound-Drum correction is implemented and locally validated. A physical Deluge re-test remains: the deliberate long-Select menu route must reach Manual Slicer without timing dependence and a slice pad must audition the selected sample.
- Decision: eligibility is defined by the selected Sound Drum plus the absence of any Drum above it. The selected row receives slice one; later slices are appended above it. Existing rows below stay in place.
- Clarification: "above" is directional, not a request to shift the Kit. The selected row remains the first generated slice; no existing pad or row below it is displaced. The selected row may already have a sample.
- Clarification: Load all and Region Slice now share this rule in P21. This plan records the Manual Slice direct-entry and audition behavior.
- Safety decision: a selected MIDI or Gate Drum, or any assigned Drum above the selected Sound Drum, rejects the action. The old temporary browser-anchor identity is removed because it incorrectly blocked the intended populated-row workflow.
- Revision: amend the existing `nl/patch-sample-slicer-04` rather than create another Sample Slicer patch, because this corrects its eligibility boundary before physical testing.
- Result so far: `SampleBrowser` checks from the row above the selected Sound Drum. `Slicer` already uses that Sound Drum for slice one and adds later generated drums above it. Choosing Manual Slice retains the selected file for its waveform and audition before stopping the separate browser preview.
- Next action: re-test the normal long-Select menu route, slice-pad audition, placement from a populated row, cancellation, and saved-kit reload on the physical Deluge.

## Work Log

- [[2026-08-22]]: Implemented P18 in the existing Manual Slice source area. Eligibility accepts only the selected new empty Sound Drum with no assigned row above it. Manual slice creation keeps that row as the first slice, assigns later generated drums to rows above it, and avoids the Region Slice allocator. The `DEVIATIONS.md` Manual Slice contract now covers this behavior.
- [[2026-08-22]]: Reviewed the complete revision before bookmarking it as `nl/patch-sample-slicer-04` at `b91fa2c07374`. The revision contains only the Manual Slice reuse behavior and its contract, then a fresh empty Jujutsu change was opened for the separate Panic follow-up.
- [[2026-08-22]]: Reopened after the anchor's newly assigned Sound Drum incorrectly counted as existing Kit content. The eligibility check must start at the selected pad plus one, preserving the selected anchor while still rejecting an existing row above it.
- [[2026-08-22]]: Physical test found that the direct entry could be timing-sensitive and that Manual Slicer pads were silent. Source review found the selected browser preview is stopped before the Manual Slicer has an independent sample source. Fold a direct-entry source-retention fix into P18; preserve the selected-anchor plus-one eligibility rule.
- [[2026-08-22]]: Folded the direct-entry correction into `nl/patch-sample-slicer-04` at `f5076438`. It loads the selected file onto the selected anchor, establishes the Slicer waveform from that source, then stops browser preview. A local Nix Release build passed and the configured CTest suite passed 7/7. Hardware behavior remains unverified after this correction.
- [[2026-08-22]]: Corrected the eligibility model after physical testing clarified that a selected populated Sound Drum is intentionally slice one. `nl/patch-sample-slicer-04` at `c2ee49beb216` now checks only that every row above is empty, removes the obsolete temporary-anchor state, passes the configured CTest suite 7/7, and regenerates the local Release ELF/BIN at 11:56. Hardware behavior remains unverified after this correction.

## Validation

- Local Nix Release target: passed after the selected-Sound-Drum correction. The local `deluge` target completed successfully at 11:56.
- Host CTest suite: passed 7/7 after the selected-Sound-Drum correction.
- Physical-device source evidence: the prior direct Manual Slice entered with a silent pad audition, and later testing established the desired populated-row workflow. Re-test the normal long-Select route, sample audition, selected-row placement, cancellation, and save/reload with the new local build. No firmware was flashed, installed, published, or submitted upstream.

## Unfinished Work

- [x] Retain the selected sample on the selected anchor before direct Manual Slice stops browser preview, update the contract, build, review, and fold the correction into `nl/patch-sample-slicer-04`.
- [ ] On a physical Deluge, test the normal long-Select menu route, selected-sample slice audition, eligible selected Sound Drum, rejected higher assigned row, Back cancellation, and save/reload before making any flashing decision.
