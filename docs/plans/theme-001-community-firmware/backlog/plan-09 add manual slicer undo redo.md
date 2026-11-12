---
description: "Active plan to add reliable local undo and redo for Manual Slicer boundary edits."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Manual Slicer undo and redo

## Purpose

Let a user safely try slice boundaries, then reverse or reapply the result without leaving Manual Slicer. This plan began on [[2026-08-22]].

## Scope

- In scope: local undo and redo for Manual Slicer boundary additions, moves, deletions, and per-slice transposition; clear feedback when an action succeeds or no corresponding history exists.
- Keep stable: the existing Save + Pad deletion gesture, ordinary Manual Slicer edits and previews, standard Slicer modes, kit playback, saving, MIDI, global song undo behavior, and hardware installation behavior.
- Out of scope: changing or deleting sample audio, a history shared with other editors, restoring an already saved project after reopening it, flashing, installation, publishing, or upstream submission.

## Questions to resolve

- Which physical controls fit established Deluge undo/redo conventions without conflicting with Manual Slicer navigation or the planned preview-stop gesture?
- What constitutes one history action when a slice point is moved by several encoder detents?
- How many boundary states can the device retain safely, and when must redo history be discarded?

## Acceptance Criteria

- [ ] Undo restores the immediately preceding Manual Slicer state for an added, moved, deleted, or transposed slice while leaving sample audio unchanged.
- [ ] Redo reapplies the last undone state and becomes unavailable after a new edit.
- [ ] The interaction provides clear feedback for successful undo/redo and for empty history without leaving Manual Slicer.
- [ ] The history is confined to the current Manual Slicer session and does not alter global undo behavior or saved project data.
- [ ] `DEVIATIONS.md` records the behavior as a self-contained, implementation-independent contract.
- [ ] Focused tests where the source seam permits, plus host unit tests and a local Nix Release build, pass. No flashing or publication occurs.

## Current State

- Status: backlog.
- Findings: Manual Slicer is an overlay and clears the global action log when focused, so existing song-level undo cannot operate inside it. The normal root-view controls, Back for undo and Shift + Back for redo, conflict with Manual Slicer's Back exit and P11's proposed Shift + Back preview stop. Manual boundary insert, Save + Pad delete, horizontal adjustment, vertical transpose, and select-encoder removal are distinct edit paths.
- Proposal: use Shift + Horizontal Encoder press for undo and Shift + Vertical Encoder press for redo. Record each successful edit as one bounded local history entry, including the affected slice and selection before and after. A new edit after undo discards redo. The history exists only until Confirm or exit; it never enters global song history or changes saved project data.
- Decision pending: confirm the proposed encoder gestures, bounded history depth, and whether one handled encoder adjustment is the intended undo step before implementation.
- Worker: `/root/manual_slicer_undo` completed source and test-seam research; implementation is intentionally unassigned while this plan is held.
- Next action: resume only when the user asks to continue Manual Slicer undo and redo.
