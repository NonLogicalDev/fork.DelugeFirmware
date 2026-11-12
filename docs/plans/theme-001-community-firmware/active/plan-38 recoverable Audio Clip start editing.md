---
description: "Make Audio Clip start edits recoverable from Audio Clip View by exposing source audio before the current Start marker."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Recoverable Audio Clip start editing

## Purpose

Let the player trim an Audio Clip from the start and later expand it back toward the original WAV boundary without depending on Undo or leaving Audio Clip View. The interaction must fit the Audio Clip timeline: Horizontal navigates the timeline, while Select adjusts a marker explicitly selected on the grid. This plan began on [[2026-08-25]] and is active under the Deluge firmware owner.

## Current State

- Status: software-complete; Plan 38 is implemented in Jujutsu change `uptykx` at bookmark `nl/patch-audio-clip-01`, current rebased commit `662a953e2915`. Formatting, the affected ARM Release objects, focused checks, all 14 configured host tests, adversarial source review, and the current combined Release build pass.
- Next action: leave physical control, Undo/Redo, and playback validation to the operator before calling the feature device-verified.

## Task boundary

- This plan changes Audio Clip View only. The separate Waveform/Sample Marker Editor interaction is owned by Plan 39 and `nl/patch-sample-editor-02`.

## Workers

- `/root`: active owner; owns accepted scope, final integration, combined validation, owner state, and user communication.
- `/root/audio_clip_bounds_viewport`: completed implementation worker; supplied playback-relative bound arithmetic, signed viewport navigation, and focused checks.
- `/root/audio_clip_undo_playback`: completed implementation worker; supplied allocation-safe coalesced marker history and exact Clip-length consequence retrieval.
- `/root/audio_clip_controls_display`: completed adversarial reviewer; found the normal-recording guard, raw-edge reverse dead zone, coincident-marker selection, and feature-Off compatibility risks that were corrected before validation.

## Product integration

- **Existing model:** Audio Clip View draws Start at musical tick zero and End at `loopLength`. Space after End remains visible and scrollable, so End can be moved outward. Trimming Start changes the raw sample boundary and shortens the Clip, then rebases the result to tick zero. Horizontal scrolling cannot move before zero, so earlier WAV material becomes unreachable through the same editor after Undo history is gone.
- **User intent:** Start editing should be as recoverable as End editing. The player should be able to reveal source audio before the current Start and move Start earlier or later without changing the meaning of the Horizontal encoder.
- **Integrated model:** Audio Clip View gains source-backed pre-start display space only when the WAV contains recoverable material. Plain Horizontal scrolls through that space. Selecting Start or End gives unshifted Select rotation temporary ownership of that marker. Moving Start changes the raw playback boundary and musical length using the existing sample-to-tick ratio.
- **Architecture impact:** Keep the feature in `AudioClipView`, add the smallest Audio Clip-specific signed scroll allowance, and reuse the Clip-length consequence for Undo and Redo. Do not add a persistent pre-roll field, duplicate sample bounds, or change the Waveform Editor.

## Scope

### In scope

- Expose the source WAV before the current playback Start as temporary negative Clip-time display space when that material exists.
- Keep plain Horizontal as scroll and pressed Horizontal as zoom while a marker is selected.
- Let the player select the existing green Start or red End marker with its pad, then turn Select to move only that marker at the current zoom resolution.
- Make clockwise Start movement trim later and counterclockwise Start movement recover earlier material. Preserve the established End direction and behavior.
- Expand or shorten `loopLength` with the raw Start boundary so the existing sample-to-musical-time ratio remains stable.
- Support forward and reversed Audio Clips, where playback Start maps to opposite raw source edges.
- Make each continuous marker-adjustment gesture one exact Undo and Redo action that restores the raw boundary and `loopLength` together.
- Preserve active playback coherently and reject an edit without partial mutation when recording, card access, action allocation, or another unsafe state prevents a reversible edit.
- Add focused host coverage, source review, a logged Release build, and a physical-device checklist before calling the feature complete.

### Out of scope

- Changing the WAV file, deleting source audio, copying audio, or creating stored silence before the sample.
- Reinterpreting Horizontal as marker movement or requiring Select to be held while Horizontal turns.
- Changing Waveform/Sample Marker Editor navigation, rendering, bound selection, sampled Synth, Kit sample, Slicer, Arranger, Song, or Automation bounds.
- Redesigning Audio Clip End, time stretching, recording, monitoring, launch behavior, or fixed-length punch-out.
- Flashing, installation, publication, or an upstream pull request.

## Behavior contract

- Green Start and red End remain the only Audio Clip View boundary markers.
- Tapping a marker selects it and gives visible selected-state feedback. Unshifted Select rotation moves only the selected marker.
- When no marker is selected, Select rotation retains its existing Audio Output-mode behavior. Shift + Select retains its existing Output-assignment behavior and must never move a marker.
- Plain Horizontal rotation always scrolls. Pressed Horizontal rotation always zooms. Marker selection does not change either rule.
- If recoverable source audio exists before Start, scrolling left reveals its waveform as editing space outside the current Clip. It is existing WAV material, not silence and not part of playback until Start moves into it.
- Moving playback Start earlier expands the Clip from the left toward raw zero in forward playback or toward the raw file end in reverse playback. Moving it later trims using the inverse operation.
- Start cannot pass the applicable WAV edge or cross End. The final partial step reaches the exact legal source boundary without wrap, overshoot, invented silence, or a ratio jump.
- Moving Start keeps the visible source position stable enough that the waveform does not jump merely because the Clip rebases to tick zero.
- End retains its existing raw-boundary, `loopLength`, direction, and visible-space behavior.
- Marker selection clears consistently on explicit view changes and unrelated controls. An offscreen marker must not retain surprising ownership of a later Select turn.
- A continuous run of Select detents is one Undo step. Back restores the exact prior raw marker and `loopLength`; Shift + Back reapplies the exact final pair. Switching markers, leaving Audio Clip View, changing Clip, or using an unrelated control closes the gesture.
- If history cannot be created, recording or count-in is active, card access owns the data path, or the edit would exceed a legal Clip limit, neither the marker nor `loopLength` changes and existing redo history remains intact.
- During ordinary playback, commit the raw marker before applying the Clip length so playback resumes against a coherent pair. The edit must not leave an invalid play position, stale voice bound, or mismatched Arrangement ClipInstance length.

## Current source model

- `AudioClip::loopLength` is the Clip's musical duration in ticks. `sampleHolder.startPos` and `sampleHolder.endPos` are raw source positions.
- Audio Clip View converts between tick-space navigation and raw sample positions using the current raw duration and `loopLength`.
- Forward playback maps playback Start to `startPos`; reverse playback maps playback Start to `endPos`.
- Current green-pad trimming moves the playback-start raw boundary inward, shortens `loopLength` proportionally, and rebases the shortened Clip to tick zero.
- Current red-pad editing uses visible grey space after End and one extra zoom level to expand the right boundary.
- Shared Timeline scrolling clamps at tick zero and uses unsigned animation plumbing, which blocks a symmetric source-backed pre-start view.
- The existing Clip-length consequence can exchange both a raw marker and `loopLength`, but callers must find the consequence for the edited Clip rather than assume it is first after automation snapshots.
- The source file is never destructively trimmed. Recoverability comes from signed Audio Clip View navigation, outward Start movement, and exact history.

## Completion Criteria

- [x] Forward Audio Clips trim Start later and recover it earlier to the raw source edge entirely from Audio Clip View.
- [x] Reversed Audio Clips provide the same playback-relative behavior against the opposite raw edge.
- [x] Horizontal remains scroll and pressed Horizontal remains zoom with no marker-moving branch.
- [x] Select moves a marker only after explicit marker selection; ordinary and Shift-modified Select behavior remains unchanged otherwise.
- [x] Pre-start waveform space is source-backed, bounded, navigable, and excluded from playback until Start moves into it.
- [x] Start edits preserve the existing sample-to-musical-time ratio and established End behavior.
- [x] Undo and Redo restore the raw boundary and `loopLength` exactly, with one continuous encoder gesture represented as one action.
- [x] Waveform/Sample Marker Editor behavior remains unchanged by this revision.
- [x] Focused checks and adversarial review cover fast turns, partial final steps, minimum-length Clips, offscreen markers, normal recording, feature-Off behavior, and coincident markers without underflow, overflow, wrap, or latched control ownership.
- [x] Display-compatible target compilation, focused tests, all 14 configured host tests, formatting, adversarial review, and one logged Release build pass.
- [ ] A physical Deluge confirms navigation, feedback, audible playback behavior, and Undo feel before the feature is called device-verified.

## Blockers

- Physical playback continuity and control feel require the user's Deluge. Software checks cannot prove whether an active edit produces an objectionable click or position jump.
- No software blocker remains. The combined Release build is the final local software gate.

## Artifacts

- **A1: Recoverable Audio Clip start-editing contract.**
	- **Type:** Backlog implementation plan and handoff document.
	- **State:** Software-complete; physical-device validation remains.
	- **Contents:** Product model, controls, boundary rules, risks, implementation sequence, and validation matrix.
	- **Location:** This plan.
	- **SCM surface:** Jujutsu change `uptykx`, current rebased commit `662a953e2915`, bookmark `nl/patch-audio-clip-01`, directly below `nl/patch-sample-editor-02`.
- **A2: Read-only Audio Clip bounds audit.**
	- **Type:** Local source investigation.
	- **State:** Complete and reconciled into the accepted contract.
	- **Contents:** Verified Audio Clip tick/raw-boundary mapping, forward/reverse behavior, signed-scroll seams, current Start and End gestures, Undo risks, playback consequences, and unsafe states.
	- **Location:** Local Community Firmware checkout under `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware`.
	- **SCM surface:** No source revision was created or changed.

## Source entry points

- **S1:** `src/deluge/gui/views/audio_clip_view.cpp` owns marker rendering, pad selection, Start and End mutation, Select behavior, and Audio Clip scroll/zoom limits.
- **S2:** `src/deluge/model/clip/audio_clip.cpp` owns Audio Clip tick-to-sample mapping and playback-relative bounds.
- **S3:** `src/deluge/gui/views/timeline_view.cpp` owns the shared horizontal scroll clamp. Keep the general Timeline rule unchanged and add only the smallest Audio Clip-specific source-backed allowance.
- **S4:** `src/deluge/gui/waveform/waveform_renderer.cpp` owns full-screen waveform scroll input and must preserve existing non-Audio views if signed plumbing is required.
- **S5:** `src/deluge/model/consequence/consequence_clip_length.cpp` restores `loopLength` and the raw marker for Undo and Redo.
- **S6:** `src/deluge/model/song/song.cpp` applies active Clip-length changes, automation consequences, Arrangement ClipInstance updates, and playback resynchronization.

## Decisions

- **Decision:** Keep Audio Clip View and Waveform Editor distinct.
	- **Rationale:** Audio Clip View owns Clip-time playback and recoverable Start editing. Waveform Editor owns whole-sample inspection and has its own Plan 39 interaction contract.
- **Decision:** Horizontal remains Audio Clip timeline navigation only.
	- **Rationale:** Horizontal scroll and pressed-Horizontal zoom are consistent across timeline views. Marker editing must not overload that control.
- **Decision:** Select rotation moves an explicitly selected Audio Clip View marker.
	- **Rationale:** It preserves both Horizontal gestures and leaves ordinary Select and Shift + Select Output behavior unchanged when no marker is selected.
- **Decision:** Show recoverable source audio as temporary pre-start display space rather than stored negative time or silence.
	- **Rationale:** The WAV already contains the material. The missing capability is access from Audio Clip View, not another persistent timing field.
- **Decision:** Preserve the sample-to-musical-time ratio while moving Start.
	- **Rationale:** Start recovery must invert the existing trim operation without silently changing time-stretch behavior.
- **Decision:** Use the existing Clip-length consequence and coalesce a continuous encoder gesture.
	- **Rationale:** Recoverability requires exact restoration of both coupled values without consuming one Undo entry per detent.
- **Decision:** Keep the feature in `nl/patch-audio-clip-01`.
	- **Rationale:** This is one Audio Clip View interaction and model correction. Waveform Editor changes use the separate `sample-editor` group.

## Implementation Steps

- [x] Record the user's interaction constraints and the read-only source findings.
- [x] Create a fresh empty Jujutsu change in the Audio Clip component group; do not use Git.
- [x] Separate the Waveform Editor interaction into Plan 39 and `nl/patch-sample-editor-02` before integrating source.
- [x] Add an Audio Clip-specific pre-start scroll domain derived from recoverable raw source duration without weakening shared Timeline clamps.
- [x] Generalize Start mutation into a signed, saturating playback-relative operation that updates the raw boundary and `loopLength` together for forward and reverse playback.
- [x] Route unshifted Select rotation to the selected Audio Clip View marker while preserving ordinary Select, Shift + Select, Horizontal scroll, pressed-Horizontal zoom, and current End behavior.
- [x] Keep the viewport visually anchored across Start rebasing and make Start and End selection cleanup consistent.
- [x] Use the dedicated marker-edit action, establish its exact Clip-length consequence before mutation, coalesce continuous turns, and close it on deterministic interaction boundaries.
- [x] Preserve coherent active playback and reject recording, count-in, card, allocation, and legal-limit failures without model or history changes.
- [x] Add focused viewport, arithmetic, wide-boundary, saturation, and compatibility checks; cover UI routing, history, recording guards, feature-Off behavior, and coincident markers through target compilation and adversarial source review where the host harness cannot instantiate the firmware UI graph.
- [x] Add the self-contained behavior to the top revision's `DEVIATIONS.md` without exposing implementation code.
- [x] Run the one uninterrupted logged Release build after the focused forward/reverse checks, configured host tests, formatting, adversarial review, and affected ARM compilation have passed.
- [ ] Leave flashing and physical-device validation to the operator.

## Work Log

- [x] [[2026-08-25]]@00:47 - Recorded the handoff after a read-only Audio Clip bounds audit.
	- **Evidence:** The audit traced the current marker, encoder, raw-boundary, `loopLength`, reverse-playback, zero-scroll, Undo, and active-length-change paths. It confirmed that current Start trimming does not alter the WAV but becomes inaccessible through Audio Clip View after Undo history is unavailable.
	- **Effect:** The queued feature now has a self-contained behavior contract: Horizontal scrolls through source-backed pre-start space, and Select rotation moves an explicitly selected marker. No firmware source, Jujutsu revision, build, flash, or publication occurred.
	- **Next:** A separate implementation task should verify the current Jujutsu stack, create the Audio Clip revision, and implement the contract from the source entry points above.
- [x] [[2026-08-25]]@00:54 - Accepted the handoff as the owner task and began current-state revalidation.
	- **Evidence:** `README.md` records owner thread `01a009d3-b4ef-7372-9753-e980d3eda5d5`, which matches the current `CODEX_THREAD_ID`. The operator explicitly selected this plan as the new task.
	- **Effect:** The behavior contract remains unchanged and is now active. No firmware source, Jujutsu revision, build, flash, publication, or Git operation has occurred.
	- **Next:** Move this plan to `active/`, record distinct read-only audits, and inspect the current Jujutsu stack before creating the feature revision.
- [x] [[2026-08-25]]@00:56 - Created the isolated Audio Clip feature revision before source edits.
	- **Evidence:** The current stack had no `audio-clip` bookmark. Jujutsu created empty change `uptykx`, commit `7423e9`, bookmarked `nl/patch-audio-clip-01` directly above `nl/patch-sample-editor-01`.
	- **Effect:** P38 has its own component revision and three distinct read-only audits are active. No firmware source, build, flash, publication, or Git operation has occurred.
	- **Next:** Reconcile the bounds/viewport, Undo/playback, and controls/display audits into the accepted design before editing source.
- [x] [[2026-08-25]]@01:02 - Paused implementation for the operator's nested editor steering.
	- **Evidence:** The operator specified a sample-information default surface, Select entry into a graphical editor, Back return, simultaneous rendering of both bounds, and Shift + Select bound switching. Current firmware has distinct `AudioClipView`, `SoundEditor`, and `SampleMarkerEditor` layers, so applying the steering to the wrong layer would change a different product surface.
	- **Effect:** The empty `nl/patch-audio-clip-01` revision remains untouched. All three audits were redirected to map the UI boundary and control consequences before the plan contract or source changes.
	- **Next:** Resolve whether this replaces P38's Audio Clip View design or becomes a separate Waveform Editor feature, then update the relevant plan before implementation.
- [x] [[2026-08-25]]@01:16 - Temporarily accepted the Sample Marker Editor as the P38 interaction model; superseded by the operator's two-task correction at 01:32.
	- **Evidence:** Three independent source audits agreed that the existing graph already owns full-source navigation and dual-bound rendering. They also found that direct WAVEFORM entry skips its Sample parent, raw Audio Clip marker writes do not update `loopLength` or history, and the rejected Audio Clip View model would duplicate selection state and require shared signed-timeline changes.
	- **Effect:** This interpretation paused Audio Clip View work but is no longer current. The 01:32 entry restores P38 and assigns the Waveform behavior to Plan 39.
	- **Next:** Implement navigation and selection first, then the Audio Clip-specific marker and length transaction with exact history.
- [x] [[2026-08-25]]@01:32 - Separated the operator's two editor tasks after explicit correction.
	- **Evidence:** The operator stated that Audio Clip View and the Waveform/Sample Editor are distinct and named two tasks: apply Plan 38, and separately fix Waveform Editor entry, return, dual-bound rendering, and Shift + Select switching.
	- **Effect:** P38 again owns recoverable Start editing in Audio Clip View. Plan 39 owns the Waveform Editor fixup. Navigation changes are being moved to `nl/patch-sample-editor-02`; Audio Clip arithmetic and history support remain in `nl/patch-audio-clip-01`.
	- **Next:** Split the shared working change by file with Jujutsu, then continue each plan independently.
- [x] [[2026-08-25]]@02:06 - Completed Plan 38 implementation and corrected the final adversarial edge cases.
	- **Evidence:** Jujutsu change `uptykx` contains the Audio Clip View, signed timeline, waveform input, action-history, consequence, pure-boundary, and focused-spec changes. Formatting passes; the affected ARM Release objects compile; the complete configured host suite passes 13 of 13. Review found and the final code corrects normal-recording edits, fast-turn reversal at a rounded raw source edge, coincident Start and End pad ownership, and disabled-Community-Feature compatibility.
	- **Effect:** Audio Clip View can reveal source-backed pre-Start material and move explicitly selected Start or End boundaries with Select while preserving Horizontal navigation, forward and reverse behavior, exact coupled history, and rejected-edit safety. Plan 39 remains a separate child revision.
	- **Next:** Complete the shared deviation contract and one uninterrupted combined Release build, then hand the physical checks to the operator.
- [x] [[2026-08-25]]@02:09 - Passed the combined Plan 38 and Plan 39 Release gate without restarting the build.
	- **Evidence:** The single Nix Release process ran from 02:08:18 through 02:09:50 PDT and exited zero after 228 steps. It produced `build/Release/deluge.bin`, 1,718,640 bytes, SHA-256 `808d2bdf8f4941bde4b826aef3e0895a1fc5d88d5c983cd3790661b4d9411af7`. The final code-only format check and all 13 configured host tests pass. The redirected log is `/private/tmp/deluge-p38-p39-release.log`.
	- **Effect:** Every local software gate in this plan passes. No firmware was flashed, installed, published, or submitted, and no Git command was used.
	- **Next:** Physically verify source recovery, forward and reverse marker feel, fast-edge reversal, active playback, rejected recording states, coincident markers, feature On and Off, and repeated Undo/Redo.
- [x] [[2026-08-25]]@02:22 - Revalidated Plan 38 as a separate Audio Clip revision after the operator restated the two-task boundary.
	- **Evidence:** Jujutsu still shows `nl/patch-audio-clip-01` as the parent of the independent Waveform Editor change. Its diff contains only Audio Clip View, signed timeline and waveform navigation, action history, consequences, boundary arithmetic, and focused tests. The refreshed formatter check and all 13 configured host tests pass. The existing Release BIN still has the recorded size and SHA-256.
	- **Effect:** No source or revision change was needed. Plan 38 remains software-complete and separate from Waveform/Sample Marker Editor behavior.
	- **Next:** Leave physical control, playback, and Undo/Redo validation to the operator.

## Validation

- **Command or action:** Focused forward and reverse marker tests.
	- **Expected result:** Start and End move inward and outward in playback-relative directions while preserving the anchored stretch ratio, reach exact legal file edges, and never cross.
	- **Observed result:** The focused arithmetic and navigation checks pass. They cover forward and reversed bounds, exact raw edges, cumulative ratios, long source positions, sequence and minimum-length limits, negative-scroll bounds, signed zoom alignment, viewport reanchoring, and the rounded raw-edge reverse regression.
	- **Failure signals:** Unsigned wrap, raw-boundary overshoot, invented silence, stretch-ratio drift, a one-detent dead zone, or different playback-relative behavior in reverse.
	- **Interpretation:** Passing will establish boundary arithmetic, not device control feel.
- **Command or action:** Audio Clip control-routing and history review.
	- **Expected result:** Horizontal only scrolls or zooms, explicit Start or End selection gives unshifted Select temporary marker ownership, one gesture coalesces into one exact Undo step, and every unsafe rejection leaves model and history untouched.
	- **Observed result:** Adversarial source review and affected ARM compilation pass. The host harness does not instantiate AudioClipView or ActionLogger, so control routing, coalescing, allocation failure, automation ordering, normal-recording rejection, and feature-Off behavior are source-reviewed rather than claimed as host integration tests.
	- **Failure signals:** Horizontal moves a marker, ordinary or Shift-modified Select loses its old role, one Undo is required per detent, Redo restores only one coupled value, automation is destructively trimmed during repeated replay, or a rejected edit consumes Redo.
	- **Interpretation:** The software paths are compiled and reviewed; physical use remains the required end-to-end interaction check.
- **Command or action:** Playback, build, and device checks.
	- **Expected result:** Stopped and active edits remain stable; unsafe recording/card states retain their guards; OLED and 7SEG builds and the configured host suite pass; the physical Deluge gives clear marker feedback without objectionable jumps or stuck playback.
	- **Observed result:** The affected ARM Release objects and all 14 configured host tests pass. The latest full-stack Release build produced the current recorded BIN. Physical hardware checks remain pending.
	- **Failure signals:** Crash, hang, runaway playback, click beyond existing length-edit behavior, cursor or waveform jump that loses orientation, stale marker state, build regression, or divergent display behavior.
	- **Interpretation:** Software gates prove buildability and deterministic state changes. Only the physical check proves the interaction is comfortable and audibly acceptable.

## Unfinished Work

- [x] Implement and locally validate the accepted behavior in an isolated Audio Clip component revision short of the combined Release build.
- [ ] Validate the control and playback behavior on a physical Deluge before any flashing decision or completion claim.

### Physical check

- [ ] Use an Audio Clip whose Start has already been trimmed. Plain Horizontal must scroll left into real earlier waveform; pressed Horizontal must still zoom.
- [ ] Tap green Start, then turn Select both ways. Counterclockwise must recover earlier audio; clockwise must trim later; red End must not move.
- [ ] Undo once with Back and redo with Shift + Back. Each must restore the exact Start and Clip length from the whole encoder gesture.
- [ ] Repeat while playing and with a reversed Audio Clip. Playback must remain coherent, and reversed movement must still be playback-relative.
- [ ] With no marker selected, confirm ordinary Select and Shift + Select keep their prior Output controls.

Related: O1 [[Synthstrom Deluge learning owner]].
