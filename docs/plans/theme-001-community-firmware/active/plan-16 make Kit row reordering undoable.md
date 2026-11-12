---
description: "Active plan to preserve undo and redo when reordering Kit rows."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Make Kit row reordering undoable

## Purpose

Let a player undo and redo the established Kit-row reorder gesture without losing unrelated undo history.

## Product integration

- Existing model: in Kit Clip View, holding two Audition pads and pressing the Vertical encoder moves the first-held row to the second-held position. The implementation directly reorders the row vector and clears every undo entry.
- User intent: retain the convenient reorder gesture while making an accidental move reversible, without treating it as a destructive project-wide operation.
- Integrated model: one successful reorder records one normal reversible action. Undo returns the exact row sequence to its prior order; Redo reapplies the same move. The moved row retains its Sound Drum, notes, row settings, and identity throughout.
- Architecture impact: use the firmware's existing action and consequence mechanism for a focused Kit-row position change rather than clearing the log or snapshotting the entire Kit.
- Existing behavior preserved: the two-Audition-pad gesture, source-to-destination move direction, row shifting, audition cleanup, display refresh, and all sound and note content stay unchanged.

## Scope

- In scope: record the row-reorder position change; undo and redo it within the current clip; retain preceding undo entries; give normal undo failure behavior when no move was made.
- Keep stable: copy/paste gestures, ordinary row length and rotation controls, MIDI and Gate rows, Kit master state, row content, save format, playback, flashing, installation, publishing, and upstream submission.
- Out of scope: a general drag-and-drop reorder UI, cross-clip or cross-Kit reordering, adding undo to Sound Drum replacement, restoring old history that was previously cleared, or changing the separate Manual Slicer undo/redo backlog.

## Completion Criteria

- [x] A successful two-Audition-pad plus Vertical-encoder reorder creates one undoable action without clearing prior undo history.
- [x] Undo restores the exact pre-move row order and Redo reapplies the same order, with every moved row retaining its sound, notes, and settings.
- [x] No action is recorded when the existing gesture does not identify two distinct Kit rows.
- [x] Existing row-reorder direction, shift behavior, audition cleanup, display refresh, and unrelated Kit controls remain unchanged.
- [x] `DEVIATIONS.md` describes the undo/redo contract without source-level instructions.
- [x] Focused source review, configured host tests, and a local Nix Release build pass. No flashing or publication occurs.

## Blockers

None. The repository has existing reversible row-edit consequences but no row-position consequence; the needed behavior is a narrow addition.

## Artifacts

- **A1:** Undoable Kit-row reorder source change.
	- **Type:** Firmware source and local Jujutsu revision.
	- **State:** Implemented and locally validated.
	- **Contents:** One reversible row-position action for the established Kit Clip View reorder gesture.
	- **Location:** `src/deluge/gui/views/instrument_clip_view.cpp` and the relevant action/consequence source.
	- **SCM surface:** `nl/patch-undo-01` at `8d3c461a08eb`.

- **A2:** Self-contained deviation contract.
	- **Type:** Behavior specification.
	- **State:** Implemented.
	- **Contents:** Observable undo and redo behavior, retained history, and compatibility boundaries.
	- **Location:** `DEVIATIONS.md` in the newest local revision.
	- **SCM surface:** `nl/patch-undo-01` at `8d3c461a08eb`.

## Decisions

- **Decision:** Use one focused reversible reorder action rather than clear the undo log.
	- **Rationale:** A row move has an exact inverse and does not need a full Kit snapshot. Retaining previous history meets the user's intent with less state and avoids unrelated data loss.

- **Decision:** Keep Undo in its own `nl/patch-undo-01` revision.
	- **Rationale:** The user requested a distinct revision for the focused, cross-feature Undo consequence even though the gesture remains in the Kit-row component.

## Implementation Steps

- [x] Confirm the current reorder gesture clears the undo log immediately before its direct row-vector move.
- [x] Trace the smallest compatible action and consequence pattern, including clip and row identity across Undo and Redo.
- [x] Add the reversible reorder action, remove the blanket undo-log clear, and preserve existing gesture behavior.
- [x] Update the deviation contract and standing owner guidance, then record the source change in `nl/patch-undo-01`.
- [x] Review the final diff, run the configured host suite and local Nix Release build, and record the exact results.

## Work Log

- [x] [[2026-08-22]] - Started P20 after the user questioned why Kit-row reordering clears undo history.
	- **Evidence:** `InstrumentClipView` calls `actionLogger.deleteAllLogs()` immediately before `noteRows.repositionElement(...)`. Existing row-edit consequences demonstrate the firmware's reversible action model, but no row-position consequence exists.
	- **Effect:** The behavior is classified as a missing undo record, not an inherent limit of row reordering.
	- **Next:** Design and implement the smallest reversible position action, then fold it into the existing Kit-row revision.

- [x] [[2026-08-22]] - Implemented the reversible row-position consequence and preserved normal action history.
	- **Evidence:** `nl/patch-undo-01` at `8d3c461a08eb`; focused source review found the stored indices reverse correctly without retaining invalidated row pointers.
	- **Effect:** A reorder no longer clears unrelated Undo history.
	- **Next:** Confirm the held-pad gesture, Undo, Redo, and retained earlier history on a physical Deluge.

## Validation

- **Command or action:** Focused source review of the move and its inverse, configured host CTest suite, and local Nix Release build.
	- **Expected result:** Undo and Redo use normal action history, source compiles, and existing host checks retain their passing result.
	- **Observed result:** Source review passed; the configured host CTest suite passed 7/7 tests; the local Nix Release build produced `build/Release/deluge.elf` after the final source change.
	- **Failure signals:** A move clears prior history, Undo changes row content instead of only order, Redo differs from the original move, compile failure in action ownership, or host-suite regression.
	- **Interpretation:** Local checks prove source integration and existing host health. A physical Deluge test is still required for the held-pad gesture and actual Undo/Redo controls.

## Unfinished Work

- [x] Implement and locally validate one undoable Kit-row reorder action.
- [ ] On a physical Deluge, move a populated Kit row, Undo, Redo, then verify notes, sound settings, and preceding undo history remain intact. Do not flash automatically.
