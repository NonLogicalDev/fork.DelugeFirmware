---
description: "Make held-Horizontal preview-only playing release-order safe in Clip View and available in every note-producing Keyboard View layout."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Make preview-only note input release-order safe

## Purpose

Let the player hold the Horizontal encoder and play pads audibly without recording those notes, with balanced recording behavior in both Clip View audition rows and Keyboard View. This plan began on [[2026-08-30]].

## Current State

- Status: software-complete; physical validation remains.
- Result: `nl/patch-note-input-01`, Jujutsu change `ruwsmuworlnvkrovryntpqnnmmnsppoz`, commit `e50345cedaa0ae0fd90afc736c593bffa585eea5`, retains preview or recorded ownership from each note-on or retrigger through its matching note-off in Clip View audition rows and all shared Keyboard layouts. An adversarial review found and drove correction of a Note Row Editor synthetic re-audition defect, then passed the repaired diff. The focused lifecycle spec passed 7 of 7, the configured host suite passed 19 of 19, formatting and affected ARM objects passed, and the single 146-step Release build completed in 67.69 seconds.
- Next action: physically test Clip View, melodic Keyboard, Velocity Drums, Note Row Editor, MIDI-Out, and CV release ordering before claiming device verification.

## Product integration

- **Existing model:** every Keyboard layout produces a shared active-note state. Melodic layouts audition and record directly; Velocity Drums delegates Kit audition and recording to Instrument Clip View. Horizontal is already a Keyboard layout modifier but does not affect recording.
- **User intent:** use the same physical shortcut as the sequencer audition column to try notes and drums during playback without writing them into the Clip.
- **Integrated model:** Horizontal held when a playable note begins marks that gesture as preview-only. It still sounds and sends ordinary output, but its complete recording lifecycle is suppressed until the matching note-off. A note begun normally retains its complete recording lifecycle even if Horizontal is pressed later.
- **Architecture impact:** add stable recording intent to Clip View audition rows and the shared Keyboard transition path. Pass explicit recording intent through the Kit audition helper instead of relying on changing global UI mode or duplicating layout logic.

## Scope

### In scope

- Instrument Clip View audition-column pads for Kit, Synth, MIDI-Out, and CV Clips.
- Physical main-grid note pads in every `KeyboardScreen` layout that emits instrument notes, including melodic, chord-generated, MIDI-Out, CV, and Velocity Drums notes.
- Audible note-on and note-off, normal MIDI/CV output, Kit Drum selection, row selection, chord display, resampling, and existing Horizontal encoder layout controls.
- Suppression of recorded note-on, count-in early note, retrigger recording, and recorded note-off for preview-owned notes.
- Stable behavior if Horizontal is released before the pad, pressed after an ordinary note begins, or used while notes overlap or remap.
- Focused host checks where a small test seam is defensible, affected ARM compilation, formatting, adversarial review, one logged Release build, and a self-contained `DEVIATIONS.md` contract.

### Out of scope

- External MIDI input, note-grid editing pads, a saved setting, flashing, installation, publication, or an upstream pull request.

## Behavior contract

- Hold Horizontal before pressing an audition-column or Keyboard note pad to hear the complete note without adding or extending sequencer notes.
- Preview ownership is decided for each note-on or retrigger and remains attached through its matching note-off. Releasing Horizontal first cannot create a stray recorded note-off.
- Pressing Horizontal after an ordinary recorded note-on does not convert that note into a preview and cannot suppress its required recorded note-off.
- Preview-owned MIDI-Out and CV notes still transmit normally; only Deluge Clip recording is suppressed.
- Chord-generated notes and Velocity Drums follow the same rule as direct melodic notes.
- Plain audition-column and Keyboard playing and recording remain unchanged when Horizontal was not held at note-on.
- Horizontal turning, pressed-Horizontal layout controls, Scale-modified Velocity Drums controls, selection, display feedback, resampling, and sounding-note cleanup remain unchanged.

## Completion Criteria

- [x] Preview-only notes follow the ordinary audition/output path in Clip View, melodic Keyboard layouts, and Velocity Drums while recording no note-on, early-note, or note-off data.
- [x] Both release orders and pressing Horizontal after an ordinary note preserve a balanced recording lifecycle in the reviewed source and focused policy checks.
- [x] Chords, same-pitch retriggers, overlaps, encoder-driven note remapping, Stop, exit, and Note Row Editor synthetic re-audition retain bounded ownership in source review.
- [x] Unmodified Keyboard recording and every existing Horizontal encoder layout action remain unchanged in the reviewed source.
- [x] Formatting, focused checks, configured host tests, affected ARM compilation, adversarial review, and one uninterrupted logged Release build pass.
- [ ] Physical Deluge testing confirms sound, recording suppression, release order, drums, piano-style layouts, and control feel before device verification is claimed.

## Blockers

- None for software implementation. Physical verification requires the operator's Deluge.

## Artifacts

- **A1: Keyboard preview-only feature plan.** This file owns the behavior and validation contract.
- **A2: Firmware revision.** Jujutsu bookmark `nl/patch-note-input-01`, change `ruwsmuworlnvkrovryntpqnnmmnsppoz`, commit `e50345cedaa0ae0fd90afc736c593bffa585eea5`.
- **A3: Repository contract.** `AGENTS.local.md` plus deviation 24 in `DEVIATIONS.md` are complete.
- **A4: Release artifacts.** `build/Release/deluge.bin` is 1,736,000 bytes with SHA-256 `0af4d6bcf176b4fb5a9ddffacbd1f84c14b753063c3a761d26e7835264d92ca3`; ELF, HEX, and MAP hashes are recorded in the final work-log entry. The build log is `/private/tmp/deluge-p46-note-input-release.log`.

## Source entry points

- `src/deluge/gui/ui/keyboard/keyboard_screen.cpp` and `.h`: shared Keyboard note transitions, audition, recording, and Horizontal button state.
- `src/deluge/gui/views/instrument_clip_view.cpp` and `.h`: Clip View audition-row ownership plus the Kit audition and recording helper used by Velocity Drums.
- `tests/spec/`: focused pure-state coverage if the ownership policy can be isolated without test-only production architecture.

## Decisions

- **Decision:** sample Horizontal at each note-on or retrigger and retain preview ownership until note-off.
	- **Reason:** a live button check reproduces Clip View's release-order defects.
- **Decision:** do not enter `UI_MODE_HOLDING_HORIZONTAL_ENCODER_BUTTON` from Keyboard View.
	- **Reason:** Keyboard's held-pad remapping excludes that mode and would break existing layout controls.
- **Decision:** use one shared Keyboard policy across drums and melodic layouts.
	- **Reason:** the feature belongs to note input, not one layout or instrument type.

## Workers

- `/root`: completed plan, integration review, final validation, revision naming, and operator communication; next: physical follow-up only.
- `/root/preview_trace`: completed source trace, implementation, focused checks, the Note Row Editor repair, and affected-object validation.
- `/root/preview_review`: completed two adversarial lifecycle reviews; found the Note Row Editor synthetic-release blocker, then passed the repaired diff with no remaining P1 or P2 finding.

## Implementation Steps

- [x] Trace the exact held-Horizontal Clip View behavior and Keyboard View gap.
- [x] Lock scope, release-order semantics, and the shared product model.
- [x] Create a dedicated empty Jujutsu change and add checkout/deviation contracts before source edits.
- [x] Implement stable Clip View audition ownership, per-note Keyboard preview ownership, and an explicit Kit recording policy.
- [x] Add focused regression coverage without changing existing layout behavior.
- [x] Run adversarial review and repair accepted findings in the same revision.
- [x] Run formatting, configured host tests, affected ARM compilation, and one uninterrupted logged Release build.
- [x] Name the completed revision `nl/patch-note-input-01` and leave flashing to the operator.

## Work Log

- [x] [[2026-08-30]] - Corrected the modifier from Vertical to Horizontal and traced current behavior.
	- **Evidence:** held Horizontal enters `UI_MODE_HOLDING_HORIZONTAL_ENCODER_BUTTON`; Instrument Clip audition remains audible while `PlaybackHandler::shouldRecordNotesNow()` rejects that mode. Keyboard View consumes Horizontal locally, so melodic and Velocity Drums notes remain recordable.
	- **Effect:** the feature is now scoped as shared Keyboard note-input behavior, with explicit ownership to avoid the sequencer shortcut's release-order hazards.
	- **Next:** create the isolated revision and implement the bounded policy.
- [x] [[2026-08-30]] - Expanded the fix to both existing and new shortcut surfaces.
	- **Evidence:** the operator asked to repair both release-order cases after reviewing the current Clip View behavior.
	- **Effect:** P46 now owns stable note-on intent in Clip View audition rows and Keyboard View; the revision group is `note-input` rather than a Keyboard-only component.
	- **Next:** implement both paths together and validate their shared lifecycle contract.
- [x] [[2026-08-30]] - Implemented shared note-input ownership and passed the first software checks.
	- **Evidence:** the focused lifecycle spec passed 5 of 5, the configured host suite passed 19 of 19, formatting passed, and Keyboard Screen, Instrument Clip View, Playback Handler, and the legacy Automation caller compiled with the target ARM toolchain.
	- **Effect:** Clip audition rows and Keyboard notes choose preview or recording ownership at note-on; Velocity Drums receives the same explicit policy without changing Keyboard's Horizontal UI mode.
	- **Next:** repair the adversarial review finding before a full build.
- [x] [[2026-08-30]] - Repaired and re-reviewed Note Row Editor synthetic audition transitions.
	- **Evidence:** the first review found that a generic synthetic note-off could consume physical ownership. Entry and row-change transitions now suppress recording without consuming that ownership; editor exit closes it once. The expanded focused spec passed 7 of 7, the host suite passed 19 of 19, formatting and the affected ARM object passed, and the second review found no remaining P1 or P2 defect.
	- **Effect:** preview and ordinary gestures remain paired through Note Row Editor entry, Horizontal release, row changes, exit, and later physical release.
	- **Next:** run one unchanged-input Release build.
- [x] [[2026-08-30]] - Completed the final software gate and named the revision.
	- **Evidence:** the single logged Release process ran from 15:40:24 through 15:41:32 PDT, completed 146 of 146 steps in 67.69 seconds, and emitted only the existing RWX load-segment warning. BIN: 1,736,000 bytes, `0af4d6bcf176b4fb5a9ddffacbd1f84c14b753063c3a761d26e7835264d92ca3`; ELF: 65,450,808 bytes, `7ea70691fc97f959c61ac84aa4a47b2deb09029ed5e5073c68c7f267b9ec9ddc`; HEX: 4,882,853 bytes, `11a053f31e278d7ed3a9ff2ca3f3507d2b4a100f53984d14d2da73e0327578c5`; MAP: 21,273,977 bytes, `53191215b251ec5d7b92c79633f1bd386362b677da5a0fbf066c5e2d988f7bf1`.
	- **Effect:** `nl/patch-note-input-01` names the software-complete revision. No firmware was flashed, installed, published, or submitted, and no Git command was used.
	- **Next:** physically validate the complete interaction matrix on the Deluge.

## Validation

- **Command or action:** source trace across Instrument Clip audition, `PlaybackHandler::shouldRecordNotesNow()`, and shared Keyboard note transitions.
	- **Expected result:** confirm the existing shortcut and identify a shared Keyboard seam that preserves layout controls.
	- **Observed result:** passed. Horizontal provides audible preview without recording in the sequencer only while its UI mode remains held; Keyboard requires explicit per-note ownership.
	- **Failure signals:** using Vertical, globally changing UI mode, muting preview output, or gating only one layout.
	- **Interpretation:** implementation can proceed without changing the user-facing gesture.
- **Command or action:** focused lifecycle checks, configured host suite, target ARM object compilation, formatter, two adversarial reviews, and one logged Release build.
	- **Expected result:** preview and ordinary notes keep paired ownership across both views without changing sound output or Horizontal controls.
	- **Observed result:** passed for software. The focused spec passed 7 of 7, CTest passed 19 of 19, all affected objects compiled, formatting passed, the repaired diff passed review, and the Release image linked in 67.69 seconds.
	- **Failure signals:** a preview note records either edge; an ordinary note loses its release; a Note Row Editor synthetic transition consumes physical ownership; a note remains sounding; or a Horizontal control changes meaning.
	- **Interpretation:** software evidence covers ownership policy, source integration, host tests, target compilation, and linking. It does not establish physical input ordering or device output behavior.

## Unfinished Work

- [ ] Physically validate stopped audition, playback, recording, count-in, both release orders, Note Row Editor, drums, piano-style layouts, retriggers, MIDI-Out, CV, Stop, exit, and Panic.
