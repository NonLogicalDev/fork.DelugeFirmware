---
description: "Add deterministic MIDI actions for a practical Deluge live-looping pedalboard."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add pedalboard live-looping MIDI actions

## Purpose

Make the common live-looping workflow controllable by foot without depending on a held pad, hidden selection, or a dangerous blind toggle. This plan began on [[2026-08-24]].

## Product integration

- Existing model: global MIDI Learn exposes transport, Loop, Layering Loop, Undo, Redo, tap tempo, and a few performance actions. A particular Clip or section can also learn a fixed MIDI message.
- Missing model: there is no selection-relative target for moving between Outputs or Clips, launching the selected Clip, or safely clearing the current loop.
- Integrated model: a pedal action targets an explicit current Session Clip and its Output. Row navigation moves between Clips; Grid track navigation moves between Output columns and chooses a Clip by a documented rule. Destructive actions name the object they delete.

## Scope

### In scope

- Add global MIDI actions for the accepted pedalboard set after the target and deletion contracts are confirmed.
- Preserve every existing global-command enum value and Flash byte assignment.
- Defer commands that are unsafe during card or audio work, then revalidate their target before execution.
- Add menu labels, Note/CC/Program Change learning, persistence, feedback, tests, a Release build, and a self-contained `DEVIATIONS.md` contract.

### Out of scope

- Hardware flashing or publication.
- Deleting an entire Grid Output column as if it were one Clip.
- Changing MIDI Follow, fixed per-Clip MIDI Learn, loop length, input routing, or monitoring in the first slice.
- Source edits before the user confirms what “delete track” should delete.

## Current capability

| Action | Present now | Note |
|---|---|---|
| Play/restart, Record, tap tempo | Yes | Play and Record are toggles. |
| Normal Loop and Layering Loop | Yes | Target is a held Session Clip, otherwise `currentClip`. |
| Undo and Redo | Yes | Deferred when immediate execution is unsafe. |
| Launch/stop one known Clip | Yes | Each Clip can learn its own fixed message. |
| Launch one known section | Yes | Each section can learn its own fixed message. |
| Next/previous Output or Clip | No | No selection-relative global command exists. |
| Launch/stop selected Clip | No | Only fixed per-Clip mapping exists. |
| Explicit transport Stop | No | Play toggles and can start transport by mistake. |
| Explicit same-Output/new-Output loop | No | Existing behavior depends on arm state; Layering Loop also changes layering behavior. |
| Cancel pending loop | No | Loop currently overloads this state transition. |
| MIDI Panic | No | The local Panic routine exists but has no MIDI command. |
| Delete current Session Clip | No | Panel-only, non-undoable, and currently clears all Undo history before eligibility checks. |
| Delete Grid Output column | No | Existing Output deletion refuses any Output that still owns Session Clips or Arranger instances. |

## Effort and risk

| Proposed action | Difficulty | Risk | Reason |
|---|---:|---:|---|
| `STOP` | 2/5 | 2/5 | Small dispatch action, but it must be idempotent rather than reuse Play's toggle. |
| `PANIC` | 2/5 | 3/5 | Core routine exists; MIDI dispatch must preserve card/audio deferral. |
| Row next/previous Clip | 2/5 | 2/5 | Session Clip order already exists; selection and reveal feedback are straightforward. |
| Grid next/previous Output | 3/5 | 3/5 | Must choose the target Clip in the adjacent column and update scroll/pulse feedback. |
| Next/previous Clip within Output | 3/5 | 3/5 | Needs section-aware ordering rather than raw storage order. |
| Launch/stop selected Clip | 3/5 | 3/5 | Existing launch machinery is reusable, but target and quantized/immediate behavior must be explicit. |
| Explicit Loop on same Output | 3/5 | 3/5 | Must set one-shot overdub routing without mutating a hidden panel arm state. |
| Explicit Loop on new Output | 3/5 | 3/5 | Must separate routing choice from continuous-layering behavior. |
| Cancel pending loop | 2/5 | 2/5 | Existing cancellation exists inside Loop; it needs a deterministic wrapper and feedback. |
| Delete current Session Clip | 3/5 | 4/5 | Existing deletion can be reused only after deferred execution, target revalidation, and preflight before history is cleared. It remains non-undoable unless separately redesigned. |
| Delete whole Output/Grid column | 5/5 | 5/5 | This is a new batch transaction across Session Clips and Arranger instances, not an existing panel action. |
| Monitoring/input/source control | 4/5 | 5/5 | Remote routing mistakes can create silence or feedback; preconfiguration is safer. |

## Recommended first slice

1. Add `STOP` and MIDI `PANIC`.
2. Add a canonical Session target with Row next/previous Clip and Grid next/previous Output.
3. Add next/previous Clip within the target Output and launch/stop selected Clip.
4. Add explicit same-Output loop, new-Output loop, and cancel-pending-loop actions.
5. Add `DELETE CURRENT CLIP` only after confirming that this is what the user meant by Delete Track. Keep whole-Output deletion out of this feature.

## Completion Criteria

- [x] Existing MIDI and live-looping actions are inventoried from source and the current official guidebook.
- [x] Missing actions are classified by implementation difficulty and behavioral risk.
- [x] Clip, Output/Grid column, Row Clip, and Kit row semantics are kept distinct.
- [ ] The user confirms whether Delete Track means the current Session Clip or the whole Output/Grid column.
- [ ] The accepted command set has deterministic behavior from Session Row, Session Grid, and Clip View where supported.
- [ ] Existing MIDI assignments remain compatible across upgrade and downgrade.
- [ ] Focused checks, configured host tests, and the Release build pass.
- [ ] Physical pedal tests confirm targeting, feedback, rapid presses, cancellation, and deletion safeguards.

## Blockers

- “Delete track” is ambiguous. The recommended contract is `DELETE CURRENT CLIP`; whole Output deletion is a separate high-risk feature.

## Artifacts

- **A1: Official Guidebook 4.1 OLED.**
	- **Type:** Current official behavior reference.
	- **State:** Reviewed for foot-controller setup, Loop/Layer, MIDI Learn, monitoring, deletion, and overdub arming.
	- **Location:** [Deluge Guidebook 4.1 OLED](https://synthstrom-audible-deluge.s3.us-east-2.amazonaws.com/Deluge-Guidebook-4p1-OLED.pdf).
	- **SCM surface:** None.
- **A2: Local source audit.**
	- **Type:** Read-only code investigation.
	- **State:** Complete.
	- **Contents:** Global MIDI plumbing, Session navigation and deletion seams, persistence slots, execution hazards, and effort ratings.
	- **Location:** This plan and the local Community Firmware checkout.
	- **SCM surface:** No source revision was created.

## Decisions

- **Decision:** Do not synthesize pad presses from MIDI.
	- **Rationale:** Pad handlers also create, copy, launch, reorder, and time gestures. Dedicated command methods are safer and testable.
- **Decision:** Append global commands without renumbering existing enum values.
	- **Rationale:** Existing assignments and the persisted default Loop command depend on stable numeric values.
- **Decision:** Use accumulated navigation input and deferred destructive execution.
	- **Rationale:** A single pending-command slot would collapse rapid pedal presses; deletion cannot safely run during card/audio work.
- **Decision:** Treat `DELETE CURRENT CLIP` and whole-Output deletion as separate features.
	- **Rationale:** A Grid column is an Output that may own several Session Clips and Arranger instances. Existing firmware only deletes an empty Output.

## Implementation Steps

- [x] Audit current global, per-Clip, per-section, MIDI Follow, navigation, looping, Panic, and deletion behavior.
- [x] Identify reusable command seams and compatibility constraints.
- [ ] Confirm the destructive target and accepted first command slice.
- [ ] Create a fresh Jujutsu feature change in the existing MIDI component group.
- [ ] Implement, review, test, build, and update the top `DEVIATIONS.md` contract.

## Work Log

- [x] [[2026-08-24]]@18:23 - Completed the read-only pedalboard capability audit.
	- **Evidence:** `GlobalMIDICommand`, its menu/learn/persistence/dispatch path, Session Row/Grid selection, loop targeting, Session Clip deletion, Output deletion, and the current official guidebook were traced.
	- **Effect:** Confirmed that basic looping, Undo/Redo, and fixed Clip/section launch already exist. Deterministic targeting, explicit Stop, MIDI Panic, selected-Clip control, explicit loop routing, and safe deletion are missing. No firmware source, Jujutsu revision, Git operation, build, or flash occurred.
	- **Next:** Confirm that Delete Track means the current Session Clip, then implement the accepted first slice.

## Validation

- **Command or action:** Read-only source trace and official-guide comparison.
	- **Expected result:** Every proposed command has a defined target, existing seam, compatibility cost, and failure risk.
	- **Observed result:** Complete. The findings and ratings above distinguish current capabilities, small wrappers, selection-model work, and destructive batch operations.
	- **Failure signals:** Calling a Row Clip, Grid Output, and Kit row the same “track”; renumbering existing MIDI commands; executing delete during card/audio work; or claiming Clip deletion is undoable.
	- **Interpretation:** The safe feature boundary is clear. Physical pedal behavior remains untested because no implementation exists yet.

## Unfinished Work

- [ ] Confirm the deletion target.
- [ ] Implement and validate the accepted command slice.
