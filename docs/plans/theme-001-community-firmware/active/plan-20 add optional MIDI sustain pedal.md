---
description: "Active plan to add optional CC64 sustain-pedal handling for incoming MIDI on internal synth tracks."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add optional MIDI sustain pedal

## Purpose

Let an external MIDI sustain pedal hold notes played into an internal Deluge synth track, including the DX7 engine, while keeping today's behavior unchanged until the player explicitly enables a Community Feature setting. This plan began on [[2026-08-22]].

## Product integration

- Existing model: incoming MIDI can play an internal synth, but CC64 does not currently defer its note-offs. DX7 follows the regular internal Sound note-off path, so it also releases immediately.
- User intent: make a normal sustain pedal work when playing DX7 or another internal synth from an external MIDI controller.
- Integrated model: an opt-in Community Feature setting enables standard incoming CC64 sustain for internal synth MIDI input. A pedal value of 64 or greater holds note-offs that arrived from that MIDI input; a lower value releases only the notes the player already released. The setting is off by default.
- Architecture impact: add the smallest stateful note-release layer at the existing internal-synth MIDI input seam. Preserve MIDI Learn, sequencer notes, grid audition, external MIDI output, Kits, project data, and saved sound parameters.

## Scope

- In scope: optional CC64 handling for notes received from external MIDI by an internal Synth Sound, standard 64 threshold behavior, note-off deferral and release, safe reset when the receiving instrument is stopped or replaced, Community Feature flag UI and persistence, source tests where a host seam exists, Release build, and physical-device test instructions.
- Keep stable: CC64 behavior while the flag is off, MIDI Learn and unlearn, incoming note velocity/channel routing, sequenced notes, Song data, synth parameters, DX7 synthesis, Kits, MIDI output, USB/MIDI device configuration, undo history, and existing feature settings.
- Out of scope: half-pedal values, sostenuto, sustain for Kits/audio clips/external MIDI output, recording pedal data into clips, a new MIDI mapping format, global MIDI filtering, hardware flashing, installation, publication, or upstream submission.

## Acceptance Criteria

- [x] A new Community Feature setting, disabled by default, clearly enables incoming MIDI sustain-pedal support for internal synth tracks.
- [x] With the setting off, CC64 retains its current handling and does not defer any note-off.
- [x] With the setting on, CC64 values 64 through 127 defer external-MIDI note-offs for the current internal Synth input, and values 0 through 63 release those deferred notes.
- [x] Notes still physically held when the pedal is released continue normally; retriggering a note does not release a newer held note by mistake.
- [x] The feature applies to the DX7 engine through the ordinary internal Synth path without a DX7-specific control or file-format change.
- [x] MIDI Learn and unlearn can still capture or remove CC64 input rather than having it silently claimed by sustain handling.
- [x] Changing or disabling the feature, replacing the target instrument, stopping it, or invoking Panic cannot leave deferred notes sounding.
- [x] The configured host suite passes and a local Nix Release build produces `deluge.elf` and `deluge.bin`. No flashing or publication occurs.

## Current State

- Status: active. Source implementation and local validation are complete; physical-device behavior remains to be checked.
- Known behavior: sustain is an envelope parameter inside a Sound, but standard MIDI sustain is a separate pedal rule that changes when an external note-off becomes audible. This needs temporary input state, not a DX7 envelope change.
- Confirmed seam: `PlaybackHandler::midiCCReceived()` gives MIDI Learn first refusal, then sends ordinary CC input to each active output. `MelodicInstrument::receivedNote()` owns the existing auditioned-note map and the final internal note-off. This lets the feature defer only an already-auditioned Synth input note while preserving recording of the physical note-off.
- Reset seam: normal target replacement and Panic already call `stopAnyAuditioning()`. The new state will clear there. Turning the Community Feature off through its menu setting will release deferred Synth notes immediately on the main control path.
- Test boundary: the configured host suite does not link `MelodicInstrument`, `SoundInstrument`, or the MIDI dispatch path. This revision will use source review, the existing host suite, a Release build, and a specific device test matrix rather than a false unit test.
- Decision: use the existing Community Features framework rather than making sustain universally active. The first revision is internal-Synth input only, so it covers DX7 without changing Kits or external MIDI output.
- Revision: use the `midi-input` component group, beginning with `nl/patch-midi-input-01`.
- Worker: `/root` owns source tracing, implementation, validation, and revision management. No flashing or publication is authorized.
- Next action: on a physical Deluge, run the pedal matrix in Unfinished Work before deciding whether to flash a later local build.

## Artifacts

- **A1:** Optional MIDI sustain implementation.
	- **Type:** Firmware source and runtime-feature setting.
	- **Contents:** CC64 input handling, deferred-note state, pedal-release behavior, safe lifecycle reset, and Community Feature registration.
	- **Location:** `src/deluge/model/instrument/melodic_instrument.{h,cpp}`, `src/deluge/gui/menu_item/runtime_feature/settings.cpp`, `src/deluge/model/settings/runtime_feature_settings.{h,cpp}`, and localization files.
	- **SCM surface:** `nl/patch-midi-input-01` at `3b1236719634c2fb70bad0143f6b116d2179cf80`.

- **A2:** Deviation contract.
	- **Type:** Self-contained behavior specification.
	- **Contents:** User-visible pedal behavior, opt-in boundary, compatibility rules, and verification requirements without source-level instructions.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.
	- **SCM surface:** `nl/patch-midi-input-01` at `3b1236719634c2fb70bad0143f6b116d2179cf80`.

## Decisions

- **Decision:** Gate sustain behind a Community Feature setting.
	- **Rationale:** CC64 is currently available to existing MIDI behavior and mappings. Opt-in avoids changing a player's established MIDI controller workflow or introducing unexpected held notes.

- **Decision:** Treat CC64 as pedal state, not as a sound parameter.
	- **Rationale:** The pedal defers note release while preserving each Sound's existing envelopes and DX7 voice behavior. It must not modify saved presets or automation.

- **Decision:** Preserve MIDI Learn and unlearn priority.
	- **Rationale:** A player must still be able to assign or remove a CC64 mapping. Sustain applies only to ordinary performance input after the existing Learn path declines it.

- **Decision:** Record physical note-offs at their normal time and defer only the audible release.
	- **Rationale:** Pedal state is not recorded into clips in this revision. A recorded performance therefore retains its existing note timing, while live internal-Synth sound follows the pedal.

- **Decision:** Release deferred notes when the Community Feature is turned off through its own setting action.
	- **Rationale:** Releasing on the menu control path avoids adding note-state mutation to the audio render path and prevents a disabled feature from leaving an audible note held.

## Implementation Steps

- [x] Trace incoming MIDI CC, note-on, note-off, voice-release, MIDI-Learn, feature-setting, and reset seams.
- [x] Add the disabled-by-default Community Feature setting and localization.
- [x] Implement CC64 pedal state and deferred note-off release for the internal Synth MIDI path.
- [x] Determine that the selected state seam is not linked by the configured host suite; record source and physical validation cases instead of adding an unexercised test.
- [x] Update `DEVIATIONS.md`, build, run the configured host suite, and review the final diff.
- [x] Assign `nl/patch-midi-input-01` and retain no unrelated code in the new revision.

## Validation

- **Command or action:** Local Nix Release build and configured host suite.
	- **Expected result:** The feature-setting and MIDI path compile; the normal host suite remains green.
	- **Failure signals:** A missing MIDI-Learn bypass, deferred notes that cannot release, note retrigger corruption, or a link/configuration failure.
	- **Interpretation:** Build and tests establish source integration. A person with the physical Deluge must still validate a real pedal and DX7 patch before any flashing decision.

## Unfinished Work

- [x] Implement and locally validate the opt-in sustain path.
- [ ] On a physical Deluge with a CC64-capable controller, test pedal down/up around single notes, chords, repeated same-pitch notes, channel routing, feature off, MIDI Learn/unlearn, instrument change, Stop, Panic, DX7, and save/reload behavior. This is a later human hardware check, not a flashing instruction.

## Work Log

- [x] 2026-08-22 12:34 - Confirmed MIDI Learn returns before output CC handling, and identified the internal Synth note-off, normal stop, target-replacement, and Panic seams.
- [x] 2026-08-22 12:34 - Chose main-control-path release on feature disable to keep deferred-note mutation out of audio rendering.
- [x] 2026-08-22 21:21 - Added the default-Off MIDI Sustain Pedal Community Feature and internal-Synth CC64 path; assigned `nl/patch-midi-input-01` at `3b1236`.
- [x] 2026-08-22 21:21 - Linked the local Nix Release ELF and BIN, then passed the configured host CTest suite: 7 of 7 tests.
