---
description: "Active plan to copy a sampled Kit row and its notes to another Kit pad."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Copy and paste Kit rows

## Purpose

Duplicate a sampled Kit row to another pad with its sound configuration and note content, so a player can begin from an exact copy and then edit either row independently. This plan began on [[2026-08-22]].

## Product integration

- Existing model: holding an Audition pad selects and auditions one Kit row. Turning ◄► while it is held edits that row's length, and holding ◄► while turning rotates it. The first local revision used the Horizontal Encoder button as an explicit Kit-row copy/paste confirmation.
- User intent: copy a sample-based Kit pad, including sample start/end and its notes, to another Kit pad without reconstructing the sound by hand.
- Integrated model: Audition + Learn immediately copies the selected sampled Kit row's instrument only. A second quick Learn press while that same source remains auditioned replaces the clipboard with the instrument and its notes. Audition + Shift + Learn pastes the local clipboard onto the selected destination. An instrument-only paste preserves destination notes; a full paste replaces them.
- Architecture impact: retain the existing narrow Kit-row clipboard and clone path at the Instrument Clip View audition/input seam, but make Learn itself the action trigger. Reuse existing sound and note-row copying where it safely preserves independent destination ownership.
- Existing behavior preserved: plain Audition + ◄► continues to edit row length, held ◄► continues to rotate the row, and the existing note and automation clipboard gestures stay unchanged.

## Scope

- In scope: copy a sampled Sound Drum row's instrument only with Audition + Learn; replace that copy with an instrument-and-notes version when Learn is pressed a second time within the short double-press interval; paste an independent duplicate with Audition + Shift + Learn.
- Keep stable: source row, destination notes until paste, Kit master settings, other rows, MIDI/Gate drum behavior, plain Audition and Horizontal Encoder actions, saving, MIDI, and hardware installation behavior.
- Out of scope: copy/paste for MIDI or Gate drums, cross-kit or cross-song clipboard persistence, Kit master settings, undo/redo history, copying automation beyond the row's ordinary note content, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [ ] Holding a populated Sound Drum Kit row's Audition pad with Learn immediately copies its Sound Drum configuration without changing the source or its notes.
- [ ] A second visible, time-bounded Learn press while the same source remains held replaces the clipboard with the Sound Drum and complete row notes. An instrument-only paste preserves the destination row's notes.
- [ ] Holding a destination Kit pad's Audition pad with Learn + Shift pastes an independent Sound Drum and, when copied, its row notes. Empty and existing Sound Drum targets follow separate guarded source paths.
- [ ] Starting standard MIDI unlearn with Shift + Learn before holding Audition remains MIDI unlearn. Kit-row paste is available only when its target is already auditioned and MIDI Learn is not active.
- [x] The copied sample reference, start/end points, sound parameters, and copied notes are cloned independently. Physical-device confirmation remains pending.
- [x] Pasting to an existing destination replaces that row's sound and notes only; it does not modify the source, Kit master state, another row, or the source clipboard.
- [ ] Plain Audition + ◄► still changes row length, holding ◄► still rotates the row, and Learn + ◄► no longer triggers the local Kit-row action. MIDI and Gate rows retain their existing behavior.
- [x] Clear local copied/pasted feedback, a copy-mode hint, and unavailable-target feedback are present for both display types.
- [x] `DEVIATIONS.md` records the revised behavior as a self-contained, implementation-independent contract.
- [x] The configured host unit suite passes and a local Nix Release build produces `deluge.elf` and `deluge.bin`. No flashing or publication occurs.

## Current State

- Status: source implementation and host validation complete; physical-device validation remains.
- Findings: raw Audition + ◄► turn conflicts with current row-length editing, so it is not a safe copy gesture. Learn is normally a momentary MIDI-learn button, but it is safe to intercept only for a held sampled Kit row. The generic note clipboard copies only the visible time window and pastes relative to scrolling, so it cannot be reused for a complete Kit-row duplicate.
- Decision: make Learn the direct action. First Audition + Learn copies the instrument only. A second quick Learn press while the same source remains held replaces the clipboard with instrument and notes. Audition + Shift + Learn pastes. V1 handles sampled Sound Drum rows only and leaves horizontal row controls unchanged. Full copies include row defaults and ordinary notes, but do not copy automation or provide undo/redo.
- Worker: `/root` owns interaction integration and validation; `/root/kit_row_copy_survey` completed the clone, replacement, interaction, and test-seam survey without source edits.
- Revision: `nl/patch-kit-rows-01` at `0c6b7bc0ab99dcd02c24b4d340f8a90a57387585` contains the direct-Learn interaction, MIDI-unlearn guard, and complete clipboard implementation.
- Next action: validate the complete gesture set on a physical Deluge. Do not flash or publish.

## Artifacts


- **A1:** Local Kit-row clipboard source change.
	- **Type:** Firmware source, built and locally validated.
	- **Contents:** A deep sampled-Sound Drum clone, full-row note snapshot, destination replacement path, and narrowly scoped Learn/Audition input handling.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/views/instrument_clip_view.{h,cpp}`, `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/processing/source.{h,cpp}`, `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/processing/sound/sound.{h,cpp}`, and `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/processing/sound/sound_drum.{h,cpp}`.
	- **SCM surface:** Jujutsu revision `0c6b7bc0ab99dcd02c24b4d340f8a90a57387585`, bookmark `nl/patch-kit-rows-01`.

- **A2:** Deviation contract.
	- **Type:** Self-contained behavior specification.
	- **State:** Completed.
	- **Contents:** Observable Kit-row copy/paste, independent ownership, boundaries, and verification requirements without source-level instructions.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.
	- **SCM surface:** Jujutsu revision `0c6b7bc0ab99dcd02c24b4d340f8a90a57387585`, bookmark `nl/patch-kit-rows-01`.

## Decisions

- **Decision:** Make an independent persistent clone before changing the destination.
	- **Rationale:** `Sound` and its sample ranges own state that cannot safely be copied by assignment. Preparing sound, parameter, and note data first avoids changing a target when an allocation fails.
- **Decision:** Treat an empty auditioned target as a real destination only at paste time.
	- **Rationale:** Holding Audition on an empty Kit pad records an add-row state rather than returning an existing row. The paste path must create that exact row only after its replacement data is ready.

- **Decision:** Make first Learn sound-only and second Learn full-row.
	- **Rationale:** The direct first action is useful for reusing a sound while retaining a destination rhythm. A quick second action deliberately carries the row notes and defaults when that is the desired starting point.
- **Decision:** Clear the normal undo log after a successful paste.
	- **Rationale:** Existing Kit replacement paths do this because replacing a drum changes Kit ownership and lifetime outside the ordinary note-edit undo model.

- **Decision:** Route paste by press order and current mode.
	- **Rationale:** Shift + Learn before Audition is the established MIDI-unlearn entry. Shift + Learn after a Kit target is already auditioned is a row paste. The row handler must not consume Learn while MIDI Learn is active.

- **Decision:** Restore the Kit-row clipboard implementation from its hidden pre-rewrite revision.
	- **Rationale:** The current revision retained the declarations and direct-Learn calls but lost their definitions during an earlier rewrite, causing a link failure. The hidden revision contains the accepted clone, replacement, and cleanup implementation; restoring it is necessary to make the existing feature build without changing its contract.

## Implementation Steps

- [x] Trace the existing input, source clone, destination replacement, and test seams.
- [x] Add the local clipboard, independent sampled-Sound Drum clone path, complete-note copy, row-default copy, and UI feedback strings.
- [x] Replace the encoder confirmation with direct Learn triggers, preserving full and instrument-only copies.
- [x] Add the explicit MIDI-Learn guard to the direct Learn handler without changing either gesture's payload.
- [x] Restore the missing Kit-row clipboard and destructor definitions, then rebuild the direct-Learn implementation.
- [x] Generate localization tables if needed, build, review, and validate the revised source change.
- [x] Update `DEVIATIONS.md` and fold the revised behavior into `nl/patch-kit-rows-01`.

## Work Log

- [x] [[2026-08-22]] - Replaced the original broad copy idea with the established Learn + ◄► grammar and implemented the first local pass.
	- **Evidence:** The source survey identified the generic clipboard's visible-window limitation, a safe deep-clone requirement for `SoundDrum`, and the empty-target add-row state. The active Jujutsu change now contains the narrow clipboard and clone implementation.
	- **Effect:** The feature is ready for source generation and build validation; no device behavior is claimed yet.
	- **Next:** Generate strings, run the Release build, and inspect the final diff.

- [x] [[2026-08-22]] - Generated strings, corrected the container API mismatch found by the first final build, and completed local validation.
	- **Evidence:** `nix develop path:./.nix --command env DBT_NO_SYNC=1 ./dbt build Release -j 1 -v` completed and produced the non-empty `build/Release/deluge.elf` and `build/Release/deluge.bin`. `ctest --test-dir build/test-unit --output-on-failure` passed all 7 configured tests.
	- **Effect:** The local feature is captured by `nl/patch-kit-rows-01` at `b5081e776323`. No Git command, flashing, installation, or publication occurred.
	- **Next:** A physical Deluge check can validate the held-control feel and save/reload behavior when hardware is available.

- [x] [[2026-08-22]] - The user replaced the untested encoder-confirmation interaction with direct Learn actions.
	- **Evidence:** The user specified first Audition + Learn as an instrument-only copy, the second quick Learn press as an instrument-and-notes copy, and Audition + Shift + Learn as paste.
	- **Effect:** Reopen P17 and amend `nl/patch-kit-rows-01`; remove the local Kit-row use of the Horizontal Encoder rather than adding a competing shortcut revision.
	- **Next:** Validate the gestures on a physical Deluge.

- [x] [[2026-08-22]] - The user approved a minimal unlearn-routing hardening.
	- **Evidence:** Standard MIDI unlearn starts Shift + Learn and then selects its Audition input, while Kit-row paste starts with an already-auditioned target and then uses Shift + Learn.
	- **Effect:** The Kit-row handler declines Learn whenever MIDI Learn is active; it does not change either clipboard payload or the normal unlearn flow.
	- **Validation:** `ctest --test-dir build/test-unit --output-on-failure` passed 7 of 7 tests; the final local Release build produced non-empty `deluge.elf` and `deluge.bin`.
	- **Next:** Validate the two press orders on a physical Deluge.

- [x] [[2026-08-22]] - The first full relink exposed missing Kit-row clipboard definitions.
	- **Evidence:** The linker reports unresolved `InstrumentClipView` vtable and destructor symbols. The header declares the destructor and the direct-Learn handler calls the clipboard methods, but their definitions are absent. Hidden revision `b5081e` contains the original accepted definitions.
	- **Effect:** Restored only those missing definitions. This recovered the existing feature implementation without changing its contract.
	- **Validation:** The logged Release build linked `build/Release/deluge.elf` and regenerated `build/Release/deluge.bin`; the host suite passed 7 of 7 tests.
	- **Next:** Physical-device validation remains.

## Validation

- **Command or action:** Local Nix Release build and existing host unit suite.
	- **Expected result:** The new clone and UI code compile with the firmware; existing host tests retain their current passing result.

- **Observed result:** On [[2026-08-22]], `nix develop path:./.nix --command ninja -C build -f build-Release.ninja -j 1 deluge` linked the Release ELF and regenerated the BIN. `ctest --test-dir build/test-unit --output-on-failure` passed 7 of 7 tests. The build log is kept temporarily at `/private/tmp/deluge-kit-rows-build.log`. No Git command, flashing, installation, or publication occurred.
	- **Failure signals:** Compile failures in source ownership, row replacement, or localization generation; regressions in the existing host suite.
	- **Interpretation:** The build establishes firmware source integration and the suite establishes the pre-existing host checks. Neither replaces physical validation of held-button, display, or saved-kit behavior.

## Unfinished Work

- [ ] On a physical Deluge, validate: source Audition + Learn sound-only copy; a second quick Learn while that same source stays held produces full copy; target Audition + Shift + Learn pastes; and Shift + Learn followed by Audition still starts MIDI unlearn. Also validate empty and occupied targets, MIDI/Gate rejection, independent later edits, display feedback, and save/reload behavior. This is a later human hardware check, not a flashing instruction.
