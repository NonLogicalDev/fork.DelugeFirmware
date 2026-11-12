---
description: "Navigate the shared Song root and enabled scale types with Scale-modified encoders across scale-aware performance views."
tags:
  - note-type/project
  - status/seedling
  - domain/learning
  - deluge
  - firmware
---

# Navigate scale roots and types with Scale encoders

## Superseded

- Status: abandoned on [[2026-09-12]]. The accepted replacement is [[plan-47 open Root and Mode menu with Shift Scale]]. Do not implement the proposed Scale + encoder shortcuts as part of the menu.
- This record preserves the earlier design and its original authorization limits. The sections below are historical; current owner authority is recorded in [OWNERS.md](../../OWNERS.md).

## Purpose

Make the shared Song key fast to adjust from any scale-aware performance surface: hold Scale, turn Vertical for the tonic, or turn Horizontal for the scale type. This plan was created on [[2026-09-06]].

## Current State

- Status: backlog; no implementation has begun.
- Proposed future revision: `nl/patch-scale-controls-01`; no bookmark or revision exists yet.
- Next action: resume from the recorded owner task or explicitly transfer ownership, then move this plan to active and update owner state before source edits.

## Product integration

- **Existing model:** The Song owns one shared tonic and scale. Scale controls vary by view, and P44 [Velocity Drums velocity profiles](../active/plan-40%20add%20Velocity%20Drums%20velocity%20profiles.md) already owns both Scale-modified encoders in Velocity Drums.
- **User intent:** Change the musical key without leaving the current scale-aware performance view or navigating a menu.
- **Integrated model:** Every supported melodic surface invokes the same shared Song-key actions and feedback. Velocity Drums retains its established profile controls, while unsupported surfaces preserve their current input handling.
- **Architecture impact:** Reuse the authoritative Song key and existing view dispatch and redraw paths. Add no per-view key state, persistence field, save schema, audio event, or timer.
- **Why this fits:** One gesture has one musical meaning wherever a shared scale is relevant, without breaking the deliberate P44 Kit exception.

## Scope

### In scope

- While Scale is held, each signed Vertical encoder detent moves the Song tonic by one chromatic note; accumulated magnitude is respected and movement wraps across all 12 notes.
- While Scale is held, signed Horizontal turns cycle bidirectionally through enabled scale types; accumulated magnitude and existing scale-validity constraints are respected.
- Support melodic Instrument Clip, melodic Keyboard, melodic Clip Automation, Session Row and Grid, Arranger, Arranger Automation, and Performance views.
- Keep the shared Song tonic and scale authoritative, and refresh relevant pads plus OLED or 7SEG feedback immediately.
- Preserve existing Scale aliases, higher-priority button chords, pressed-encoder chords, sidebar and audition-column ownership, and consume the handled Scale press and release so it cannot also toggle Scale mode.
- Add focused input-policy tests, formatting, configured host tests, affected ARM compilation, one uninterrupted logged Release build, adversarial review, and a physical control matrix.

### Out of scope

- Velocity Drums, which keeps P44 Scale + Horizontal profile selection and Scale + Vertical fixed-velocity editing.
- New behavior in other Kit layouts, Audio views, sample editors, Slicer, browser, recorder, Sound Editor, or menus; their existing dispatch passes through or the new action is a no-op.
- Note retriggering, note or automation recording changes, Clip-local keys, persistence or schema changes, flashing, installation, publication, or upstream submission.

## Behavior contract

- Positive and negative Vertical deltas traverse the 12 tonics symmetrically and wrap in both directions; a delta magnitude greater than one applies every detent.
- Positive and negative Horizontal deltas traverse only currently enabled, valid scale types in their established order and wrap in both directions; a delta magnitude greater than one applies every detent.
- All supported surfaces edit the same Song tonic and scale and show the resulting value through their available OLED or 7SEG feedback and redraw paths.
- A handled Scale-modified turn owns the Scale gesture through release. Releasing Scale does not also enter, leave, or toggle Scale mode.
- Existing aliases and more specific button or pressed-encoder chords retain precedence. Sidebar and audition controls retain their established ownership.
- Velocity Drums behavior remains exactly as specified by P44. Unsupported surfaces gain no new scale action.
- Changing tonic or scale does not retrigger sounding notes, alter recorded events, start recording, or add saved state.

## Completion Criteria

- [ ] Signed tonic traversal covers all 12 notes, wraps both ways, and respects encoder-delta magnitude on every supported surface.
- [ ] Signed scale traversal visits only enabled valid types, wraps both ways, and respects encoder-delta magnitude on every supported surface.
- [ ] Gesture precedence, aliases, pressed-encoder chords, sidebar and audition ownership, and Scale-release consumption match the contract.
- [ ] Velocity Drums retains its P44 profile and fixed-velocity controls; unsupported views remain unchanged.
- [ ] OLED and 7SEG feedback plus relevant redraws show the shared Song key without retriggering notes or changing recording or persistence.
- [ ] Focused tests, formatter, configured host tests, affected ARM compilation, adversarial review, and one uninterrupted logged Release build pass.
- [ ] A physical Deluge passes the supported-view and explicit-exception control matrix.

## Blockers

- Firmware implementation is blocked by owner identity. The recorded owner task must resume or ownership must be explicitly transferred. The current narrow permission authorizes only creation of this backlog plan and does not authorize README, AGENTS, EXEC_STATE, other plan, firmware, or version-control changes.

## Artifacts

- **A1: This backlog plan.**
	- **Type:** Owner plan document.
	- **State:** Created; implementation blocked.
	- **Contents:** Accepted behavior, view matrix, P44 exception, validation plan, and ownership boundary.
	- **Location:** `track-004-community-firmware/backlog/plan-45 navigate scale roots and types with Scale encoders.md`.
- **A2: P44 Velocity Drums contract.**
	- **Type:** Existing plan and compatibility dependency.
	- **State:** Software-complete; physical validation remains.
	- **Contents:** Established Velocity Drums Scale + Horizontal and Scale + Vertical behavior that this plan preserves.
	- **Location:** P44 [Velocity Drums velocity profiles](../active/plan-40%20add%20Velocity%20Drums%20velocity%20profiles.md).
- **A3: Scale-controls firmware feature.**
	- **Type:** Future local firmware feature, tests, and self-contained deviation contract.
	- **State:** Planned; not created.
	- **Location:** Local Community Firmware checkout; implementing owner must reverify the exact path.
	- **SCM surface:** Proposed `nl/patch-scale-controls-01`; not created.

## Decisions

- **Decision:** Change the Song's shared key instead of adding per-view or per-Clip scale state.
	- **Rationale:** Every supported surface should reflect one authoritative musical context, and existing saves need no new data.
- **Decision:** Preserve Velocity Drums as an explicit exception.
	- **Rationale:** P44 already assigns both gestures to a coherent Kit-only velocity feature; silently overriding them would make that feature unreachable.
- **Decision:** Consume Scale through its matching release after a handled encoder turn.
	- **Rationale:** One gesture must not also trigger the existing Scale-mode action.
- **Decision:** Add the `DEVIATIONS.md` contract only when implementation begins.
	- **Rationale:** The behavior is planned, but no firmware revision or implementation artifact exists yet.

## Implementation Steps

- [x] Audit current Scale gesture collisions and identify the P44 Velocity Drums exception.
- [x] Record the accepted behavior and ownership blocker in this backlog plan.
- [ ] Resume the recorded owner task or explicitly transfer ownership; move this same plan to active and update owner guidance and execution state before source edits.
- [ ] Re-read checkout guidance and map the shared key action and dispatch precedence across every supported and excluded surface.
- [ ] Implement root and enabled-scale traversal, feedback, redraw, and release ownership without new persisted state.
- [ ] Add focused traversal and gesture-policy tests, then run independent review and repair accepted findings.
- [ ] Create `nl/patch-scale-controls-01`, add the self-contained `DEVIATIONS.md` contract, and run the planned software validation once implementation is authorized.
- [ ] Leave flashing and physical validation to the operator.

## Work Log

- [x] [[2026-09-05]] - Completed the read-only conflict and source audit.
	- **Evidence:** The audit found shared Song key state and the requested scale-aware surfaces, plus an exact collision with both Scale-modified encoders in P44 Velocity Drums.
	- **Effect:** The design uses one shared action and keeps Velocity Drums as an explicit exception; unsupported views gain no new behavior.
	- **Next:** Record the complete contract without changing owner or firmware state.
- [x] [[2026-09-06]]@14:35 - Created this backlog plan under one narrow owner-edit permission.
	- **Evidence:** `plan-45` did not exist in active, backlog, completed, or archived status before creation.
	- **Effect:** The requested behavior is now recoverable, while the owner mismatch remains an explicit implementation blocker.
	- **Next:** Continue only from the recorded owner task or after explicit ownership transfer.

## Validation

- **Command or action:** Signed traversal pure tests for all 12 roots and enabled scale types, including positive and negative multi-detent deltas and wrap.
	- **Expected result:** Both axes visit only their intended values in both directions and respect the full encoder delta.
	- **Observed result:** Not run; this is a backlog plan only.
	- **Failure signals:** Skipped or repeated values, disabled or invalid scales, lost delta magnitude, asymmetric wrap, or one axis changing the other's property.
	- **Interpretation:** No behavior has been validated yet.
- **Command or action:** Gesture-precedence and release-consumption tests, unchanged-P44 checks, formatter, configured host suite, affected ARM compilation, one uninterrupted logged Release build, and physical view matrix.
	- **Expected result:** Every supported surface changes the shared key; higher-priority gestures and explicit exceptions remain unchanged; software checks and physical controls pass.
	- **Observed result:** Not run; implementation is blocked.
	- **Failure signals:** Scale-mode double action, broken alias or chord, stolen sidebar or audition input, changed Velocity Drums behavior, note retrigger, recording or save-format change, stale feedback, compile or test failure, or physical-view mismatch.
	- **Interpretation:** These checks are planned evidence, not current proof.

## Unfinished Work

- [ ] Clear the owner-identity blocker and activate this plan before any source, repository-guidance, `DEVIATIONS.md`, or version-control change.
- [ ] Implement, review, validate, and physically test the complete supported-view and exception matrix.
