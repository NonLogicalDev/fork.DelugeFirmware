---
description: "Read-only comparison of looping behavior in Deluge Grid and Row Song layouts."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Investigate Grid and Row loop behavior

## Purpose

Establish why Grid View feels inconsistent or buggy for audio looping when compared with Row View. Produce a behavior matrix grounded in source and current manual behavior, distinguish deliberate product differences from confirmed defects, and identify the smallest coherent repair candidates without changing firmware. This plan began on [[2026-08-24]].

## Current State

- Status: complete. The current source still confirms the held-Clip plus short-Record cancellation defect, and the read-only report satisfies every completion criterion.
- Next action: wait for explicit operator authorization before creating a separate implementation plan or changing firmware.

## Product integration

- Existing model: Row and Grid are alternate Session/Song layouts that should expose the same underlying clip, launch, recording, and loop concepts through different pad geometry.
- User intent: Grid must be as dependable for looping as Row, not merely offer superficially similar controls.
- Integrated model: retain only layout-specific spatial mapping and affordances. For every shared audio-looping outcome, Grid should use the same eligibility, timing, state transition, and feedback contract as Row unless a documented layout reason requires a difference.
- Architecture impact: first trace the shared Session and Playback paths plus both views' input adapters. A later fix, if authorized, should belong to the responsible loop/session component rather than add divergent Grid-only behavior.

## Scope

- In scope: read-only tracing and comparison of Grid and Row entry points for launching, stopping, recording, closing a first tempoless loop, regular overdub, continuous overdub, clip arm states, immediate actions, selected-clip feedback, scroll/coordinate mapping, and external-clock or metronome gates; current manual and shortcut comparison; a concrete defect and test matrix.
- Keep stable: firmware source, Jujutsu stack, `DEVIATIONS.md`, local behavior, build outputs, project data, hardware state, and prior feature plans.
- Out of scope: implementation, UI redesign, changing intended loop semantics without evidence, hardware flashing, installation, publication, or upstream submission.

## Completion Criteria

- [x] A Grid-versus-Row behavior matrix names the input, prerequisite state, expected outcome, actual source path, and user-visible feedback for each shared looping action.
- [x] Each mismatch is classified as confirmed defect, intentional difference, or unresolved pending a physical check, with source evidence beside it.
- [x] The report identifies a smallest coherent repair boundary and physical regression matrix for every confirmed defect, without editing firmware.
- [x] Owner records accurately state that this is read-only and no firmware revision, build, or flash was created for the investigation.

## Blockers

- None. Source and current manual behavior are available. A physical Deluge is needed only to resolve any source ambiguity that depends on timing or LED feedback.

## Artifacts

- **A1:** Grid and Row loop behavior report.
	- **Type:** Source-backed investigation in this plan.
	- **State:** Complete.
	- **Contents:** Gesture and state matrix, findings, classification, repair boundaries, and device test cases.
	- **Location:** This plan.
	- **SCM surface:** None. This is read-only.

## Decisions

- **Decision:** Investigate before proposing a Grid-specific patch.
	- **Rationale:** Looping is shared Session and Playback behavior. A visible Grid problem may originate in shared state, an intentional gesture mapping difference, or a Grid adapter defect; source evidence must choose the repair boundary.

- **Decision:** Use Row View as the comparison baseline, not as an assumption that every gesture must be identical.
	- **Rationale:** Geometry can legitimately differ, but shared loop outcomes must remain explainable and reliable.

## Implementation Steps

1. [x] Map Row View's audio-loop and recording gestures to Session and Playback operations.
2. [x] Map Grid View's equivalent gestures, mode gates, coordinate conversion, and feedback paths.
3. [x] Compare both maps against the current looping manual and shortcut reference.
4. [x] Record and rank behavior mismatches; identify their exact state preconditions and likely ownership.
5. [x] Produce a targeted physical-device test matrix and, only if a defect is confirmed, propose a separate implementation plan and component-organized revision.

## Findings

### Behavior matrix

| Outcome | Row input and result | Grid input and result | Classification |
|---|---|---|---|
| Create a new recorded clip on a running clock | Hold `RECORD`, then press the pad below a source Clip. Row input finds the source at `y + 1`, creates a pending overdub, enables Record if needed, and schedules it through `Session`. | Enable `CREATE + RECORD`, enable Record before touching the grid, then short-press an empty cell in an existing Instrument column. Grid creates and launches a new Clip, which the shared Session engine arms to record on the next bar. | Deliberate layout-specific workflow. Grid does not use the Row's below-the-source gesture. |
| Begin a selected-Clip loop or layered loop | Hold the Clip, then press `RECORD`. The View dispatches the configured Global Loop command. | Select a Clip, then use an external Global `LOOP`/`LAYERING LOOP` command or enable the red/magenta Grid Loop Pads. The pads dispatch the same shared Global commands. | Same intended shared outcome, but Grid requires an opt-in control surface. |
| Close the first tempoless recording | A shared Session/Playback path measures the recording and establishes tempo when there is no clock, the metronome is off, and every active Clip is an eligible audio Clip. | The same shared path is used after Grid launches the Clip. | Matched source engine. Real input timing and LED feedback still need a device check. |
| Stop a clocked linear recording | Loop command again or use the normal Clip launch/stop path. `Session` quantizes the end to the Clip's loop length. | Loop command again or use the new Clip cell as documented. The same Session launch state machine owns the stop. | Matched state engine, different access gesture. |
| Make an audio overdub in place | Row always uses the historic cloned-Clip looper path. | Grid uses in-place looping only when the Audio Output is `Looper/FX`; otherwise it also takes the cloned-Clip path. | Deliberate audio-engine constraint, poorly surfaced in Grid. |
| Use a short physical `RECORD` press while holding a Clip | The Loop command is dispatched, then the normal Record release also runs unless the action has been explicitly consumed. Row's older `RECORD`-then-pad path does consume the button. | This is Grid's prominent direct-Clip route. A short press dispatches Loop first, then the normal Record release turns recording off and deletes pending overdubs. | **Confirmed defect.** |

### Confirmed defect: a short Clip + Record loop command cancels itself

- `SessionView::buttonAction()` dispatches a loop command while a Clip is held, but returns `NOT_DEALT_WITH`; the normal button handler therefore still owns the physical Record release ([session_view.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/views/session_view.cpp:429)).
- The button handler treats a release before `kShortPressTime` as a normal Record action ([buttons.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/hid/buttons.cpp:259)). That action turns recording off and deletes every pending Session overdub ([playback_handler.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/playback/playback_handler.cpp:294)).
- The Loop command created that pending overdub immediately before ([playback_handler.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/playback/playback_handler.cpp:3401)). So a normal quick tap both starts and cancels the loop request.
- Holding Record past the short-press threshold avoids the release action. That is an accidental workaround, not a documented interaction. The current Grid guide says to "press Record" and does not tell the player to hold it ([Looping in Grid View](https://delugecommunity.com/features/looping_in_grid_view/)).

### Important differences that are not this defect

- Grid intentionally has two looping workflows, not Row's spatial `RECORD`-then-pad workflow: `CREATE + RECORD` for an empty cell, and Global `LOOP`/`LAYERING LOOP` for a selected Clip. The Grid guide describes these as separate techniques and still says the Global-command mechanics are being finalized ([Looping in Grid View](https://delugecommunity.com/features/looping_in_grid_view/)).
- Grid Loop Pads are disabled by default, so the red and magenta right-column controls do nothing until `SETTINGS > COMMUNITY FEATURES > GRID VIEW LOOP PADS (LOOP)` is enabled ([runtime_feature_settings.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/model/settings/runtime_feature_settings.cpp:179)). This is a discoverability problem, not a state-transition failure.
- The sidebar loop-pad handler ends the held-pad UI state before invoking the Global command ([session_view.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/views/session_view.cpp:3902)). This looks suspicious, but Grid selection has already made that Clip the sticky `currentClip`; `loopCommand()` then uses it as its fallback target ([playback_handler.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/playback/playback_handler.cpp:3359)). It is fragile ownership, but source does not establish a wrong-target bug.
- Grid's `CREATE + RECORD` is disabled by default and only applies to an empty cell in an existing Instrument column ([session_view.cpp](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/views/session_view.cpp:4241)). This is a configured prerequisite, not a failure after it is enabled.
- A Grid audio Clip can overdub in place only in `Looper/FX` mode; Player and Sampler use the cloned-Clip path ([audio_clip.h](/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/model/clip/audio_clip.h:63)). The Grid does not clearly expose that distinction while looping.

### Repair boundary

- **Smallest repair:** consume the physical Record press when Session View actually dispatches its held-Clip Loop command. Do not change the global `recordButtonPressed()` behavior; it is correct for ordinary Record presses. This is a shared Session-input correction, not a Grid-only workaround, because the held-Clip + Record path is shared by both layouts.
- **Likely ownership:** `SessionView::buttonAction()` plus the Record-button consumption contract in `buttons.cpp`. The later Session and Playback scheduling logic should remain untouched.
- **Do not mix in:** Grid pad layout, Loop Pad enablement, output-mode semantics, or a Row-gesture clone. Those are separate product decisions and would hide the actual cancellation bug.

### Physical regression matrix for a later fix

1. With a clock running, enable Grid Loop Pads and configure an audio Clip with an input. Hold the Clip and quick-tap Record. The request must survive Record release, begin at the normal quantization boundary, and set the expected armed/recording feedback.
2. Repeat with Record held longer than the short-press threshold. It must produce the same loop state as the quick tap.
3. Repeat in Row View with hold-Clip + quick Record, then with Row's older Record-then-pad-below gesture. Neither may alter ordinary global Record state unexpectedly.
4. Repeat Global `LOOP` and `LAYERING LOOP` using the red and magenta Grid pads. Confirm normal Loop auto-extends and Layering Loop keeps the original length.
5. Repeat with `Looper/FX` and Player audio-output modes. Verify in-place versus cloned-output behavior is preserved, and record which visual track/cell represents a clone.
6. Test a first tempoless audio loop with metronome off, then a clocked loop with metronome on. Confirm tempo-setting, record end, and LED states on hardware.
7. Repeat while no audio input is selected, while a Clip is not armed, and while arrangement recording is active. Confirm the existing rejection feedback remains and no pending Clip is left behind.

## Result

Grid is not simply failing to redraw Row behavior. It uses a different front-end around the same Session engine, with a default-off loop-pad feature and a separate empty-cell recording flow. The critical defect is narrower and concrete: the shared held-Clip + short-Record command is not consumed, so its normal release cancels the pending loop. No firmware source, Jujutsu revision, build, or flash was created during this investigation.

## Work Log

- [x] 2026-08-24 01:39 - Created a read-only investigation after the user reported that Grid looping is buggy and does not match Row behavior.
	- **Evidence:** Grid and Row looping share the existing Session/Song feature area, while the specific mismatch is not yet identified.
	- **Effect:** No source, Jujutsu, build, or device state changed.
	- **Next:** Trace both layouts' loop entry points and state transitions.

- [x] 2026-08-24 02:12 - Completed source and current-manual comparison.
	- **Evidence:** A short held-Clip + Record press reaches `tryLoopCommand()`, then the unconsumed Record release reaches `recordButtonPressed()` and removes pending overdubs. Grid and Row otherwise converge in Session scheduling after their distinct adapters.
	- **Effect:** Classified one confirmed shared-input defect, deliberate Grid workflow differences, and hardware-only checks. No source, Jujutsu, build, or device state changed.
	- **Next:** Await a separate user-authorized implementation decision for the Record-consumption repair.

- [x] 2026-08-25 02:27 - Reverified the completed investigation against the current local stack.
	- **Evidence:** `SessionView::buttonAction()` still dispatches `tryLoopCommand()` without consuming Record, the physical-button release still invokes `recordButtonPressed()` after a short press, and that path still deletes pending Session overdubs when Record turns off.
	- **Effect:** The confirmed defect and smallest repair boundary remain current. No firmware source, Jujutsu revision, build, flash, or publication changed.
	- **Next:** Move this completed read-only plan out of `active/`; any repair still requires separate operator authorization.

## Validation

- **Command or action:** Source-path comparison, current manual review, then a focused physical Deluge matrix for any state or timing behavior source alone cannot prove.
	- **Expected result:** Every conclusion names a concrete input/state path and distinguishes verified implementation behavior from device-only observations.
	- **Observed result:** Source establishes the short-Record pending-overdub cancellation. Current public guidance confirms that Grid deliberately exposes separate Create + Record and Global Loop workflows, while warning that the latter is still being finalized.
	- **Failure signals:** Treating a gesture difference as a bug without tracing its state gate, claiming a device observation from source alone, or proposing a Grid-only repair that bypasses shared Session/Playback semantics.
	- **Interpretation:** The cancellation defect is source-confirmed. The listed device matrix is still required for LED cadence, input latency, and clone-visibility confirmation.

## Follow-up

- A later, explicitly authorized implementation should have its own plan and an input/session-component revision. It should only consume the held-Clip Loop Record gesture, then run the physical matrix above.
