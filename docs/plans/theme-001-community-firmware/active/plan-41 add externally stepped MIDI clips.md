---
description: "Prototype MIDI-Out Clips that advance from independent learned MIDI pulses instead of the Song clock."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add externally stepped MIDI Clips

## Purpose

Let the Deluge act as several independent step sequencers for VCV Rack. Each eligible MIDI-Out Clip advances from its own learned, channelized MIDI pulse rather than the Song tempo. This plan began on [[2026-08-26]].

## Current State

- Status: active only for physical follow-up; the schema-marker correction, parser hardening, automated validation, and independent review are complete.
- Result: `nl/patch-midi-sequencer-01` remains Jujutsu change `plloqspk` at commit `61315dd4`. External Step writes `nl-save-schema-1`; supported exact markers load, malformed or future local markers fail closed, and ordinary official and Community comparisons remain unchanged. The prerequisite `ns/bug-song-load-01` is change `wxsvvosv` at commit `da03c661`.
- Next action: leave physical Deluge/VCV Rack and old/new firmware load-resave checks to the operator. P48 may now extend the same schema system to 2.

## Task boundary

- This plan owns only externally stepped Session MIDI-Out Instrument Clips, their learned pulse inputs, playback state, controls, persistence, and tests.
- P39 owns pedalboard live-looping commands. It does not own external stepping; the plans must coordinate only where both touch MIDI Learn or global command routing.
- This is not a general polytempo engine and does not add channelized standard MIDI Clock. MIDI Clock, Start, Continue, and Stop are connection-wide system messages.

## Product integration

- **Existing model:** every active Clip receives the same Song tick increment. MIDI Clips have independent loop lengths and phase, but not independent rates.
- **User intent:** patch several VCV clocks into the Deluge so each MIDI sequence can move at its own regular or irregular rate.
- **Integrated model:** create or select an ordinary MIDI-Out Clip, then choose `CLOCK` → `EXT STEP` in that Clip's Settings. Internally it remains an existing MIDI Instrument Clip with a different clock mode, so note editing and MIDI output stay shared rather than creating a second Clip implementation or fake output type.
- **Architecture impact:** persist a small per-Clip clock configuration, preserve MIDI input identity through dispatch, and add a pulse-driven Instrument Clip path that does not write into the Song-wide next-event scheduler. The Song tick path must skip these Clips.

## Scope

### In scope

- Add a default-Off Community Feature for External Step MIDI Clips.
- Add `SONG` and `EXTERNAL STEP` clock modes to Session MIDI-Out Instrument Clips. Old and ordinary Clips remain `SONG`.
- Let a new Clip be created as External Step and let an eligible existing MIDI-Out Clip change clock mode without changing its notes.
- Learn separate Step and Reset pulses per Clip as `(input device or cable, MIDI channel, message type, number)`. Each binding accepts Note, CC, or Program Change. The feature is not complete without both control roles. A valid Step binding is required before the Clip can advance; Reset may remain visibly `UNASSIGNED` without blocking playback.
- Store a pulse step size per Clip. Start with `1/4`, `1/8`, `1/16`, and `1/32`; default to `1/16`.
- Preserve normal notes, probability, iterance, loop length, and stepped MIDI CC automation at pulse boundaries.
- Add clear `WAIT`, missing-input, pulse-resolution, and conflict feedback on OLED and 7SEG displays where practical.
- Use a dedicated exact binding with device/cable, raw channel, Note/CC/Program Change type, number, and transient edge state. Do not reuse `LearnedMIDI`'s wildcard-device and MPE-zone representation.
- Conditionally require local save schema 1 whenever any saved Clip remains External Step, including while the feature is Off or its input is unavailable.
- Add focused host checks, affected ARM compilation, adversarial review, a self-contained `DEVIATIONS.md` contract, and one logged Release build.

### Out of scope

- Audio, Synth, Kit, or CV Clips; Arrangement playback or recording; live note recording; overdub recording; Song Position Pointer; MIDI clock output; or multiple standard MIDI Clock domains.
- Reverse or Ping-Pong Clip direction, independent NoteRow loop length or direction, swing between pulses, sub-step timing, smooth automation, arpeggiators, MPE timing, sample sync, Fill launches, or section-repeat automation.
- Flashing, installation, publication, or an upstream pull request.

## Provisional behavior contract

### Activation and transport

- Global Play is the master enable. The Song clock never advances an External Step Clip.
- Launching an External Step Clip arms it and shows `WAIT`. The first accepted Step pulse starts at Clip position zero and emits position-zero events exactly once. Later pulses advance by one saved step.
- Relaunching a stopped Clip restarts it from position zero on its next pulse. Switching Clips on one MIDI Output stops the prior Clip's active notes before the replacement waits for its first pulse.
- Clip stop, mute, Output replacement, global Stop, and Panic immediately send owned note-offs and clear transient pulse state. Pulses received while global transport is stopped are consumed but do not move the Clip.
- Reset works whether global transport is running or stopped. It sends owned note-offs, places the Clip immediately before position zero, and does not emit a step. The next accepted Step pulse emits position zero.

### Pulse input

- For a Note binding, positive-velocity Note On is an edge. Repeated Note On while the learned note remains held does nothing; Note Off or velocity-zero Note On rearms it.
- For a CC binding, crossing from `0–63` into `64–127` is an edge. Remaining high does nothing; a later value below 64 rearms it.
- For a Program Change binding, every matching Program Change message is one edge because the message has no release state.
- Every matching Note, CC, or Program Change message used by an active Step or Reset binding is consumed. It is not auditioned, recorded, sent through MIDI Follow, mapped as a parameter, echoed through MIDI Thru, or emitted by the target MIDI-Out Clip.
- MIDI Learn has priority while learning. Learning a pulse must not also advance a Clip.
- Several active Clips may deliberately share the same Step or Reset binding. One Step edge advances every matching Clip once; one Reset edge resets every matching Clip once, allowing a VCV clock and reset line to control a group.
- A Clip cannot use the same exact device, channel, type, and number for both Step and Reset. A binding that conflicts with an existing higher-priority command or parameter mapping remains saved but inactive and reports the conflict.
- Existing command and parameter mappings keep priority. Conflict detection is Song-wide and includes inactive Outputs so a binding cannot change meaning merely because another Clip launches.
- When the Community Feature is Off or Session playback is not the active playback mode, configured controls are not consumed and do not act. The Settings submenu remains available on a persisted External Step Clip so the player can recover it to `SONG`.

### Stepping and notes

- One pulse advances exactly one saved step regardless of pad zoom or the interval since the prior pulse.
- Events on a step boundary retain their existing probability and iterance behavior. MIDI CC automation changes only at a pulse boundary; it does not interpolate between pulses.
- An off-grid event is delayed to the first pulse boundary at or after its stored position. Playback does not rewrite the stored event.
- A note shorter than one pulse sounds for one pulse. When note-offs and note-ons meet at one boundary, note-offs are emitted first.
- Loop wrap is local to the Clip. Different Clips may use unrelated pulse rates, step sizes, and loop lengths.
- External Step requires forward Clip direction, the master Clip timeline, and a loop length evenly divisible by the saved step. Enabling the mode or later edits that violate these constraints leave it inactive with explicit unsupported-content feedback; playback does not rewrite musical data.
- If several starts in one NoteRow quantize to the same pulse boundary, the latest stored start before that boundary is the single emitted note-on. Earlier collapsed starts remain saved but do not emit an additional same-pitch note.

### Clock loss and safe fallback

- A watchdog becomes active only after three consecutive pulse intervals from 25 ms through 4 seconds remain within 25 percent of their median. Four missed median intervals cut the Clip's active notes and return its display to `WAIT` without changing phase. Slower or deliberately irregular pulses still step but do not establish an automatic timeout.
- The next pulse after clock loss continues from the frozen phase. Only Stop, relaunch, or Reset returns to position zero.
- A disconnected learned input, disabled Community Feature, invalid saved binding, or unresolved mapping conflict leaves the Clip stopped. It must never fall back silently to the Song clock.
- Loading a saved External Step project in older firmware must not silently run those Clips from the Song clock. The implementation must prove a safe downgrade behavior or add a clear project compatibility guard before the feature is considered complete.
- A Song containing any External Step Clip writes the exact sentinel `nl-save-schema-1` as `earliestCompatibleFirmware`; the marker disappears only after every affected Clip returns to `SONG`. Patched firmware accepts supported local schema values and rejects malformed or future values. Unpatched firmware treats the sentinel as unknown and refuses it. Ordinary official and Community version comparisons remain unchanged.

## Completion Criteria

- [ ] Two or more External Step MIDI Clips on one USB MIDI connection advance independently from different device/channel/type/number bindings while ordinary Song-clocked Clips retain current behavior.
- [x] Note, CC, and Program Change bindings follow their defined edge and rearm behavior for both Step and Reset.
- [x] Shared Step and Reset bindings affect every matching active Clip exactly once, and no matching pulse is auditioned, recorded, followed, mapped, or echoed.
- [x] First pulse, subsequent pulse, loop wrap, off-grid event, short note, Reset, relaunch, Clip switch, global Stop, and Panic follow the contract without stuck notes.
- [x] Probability, iterance, note lengths, and stepped MIDI CC automation remain deterministic at pulse boundaries.
- [x] Missing clocks, disconnects, disabled feature state, invalid saved state, and mapping conflicts stop safely without Song-clock fallback.
- [ ] Old projects load as ordinary `SONG` Clips; new projects retain mode, bindings, and step size across save and reload; downgrade behavior is explicitly safe.
- [x] Forward/divisible-loop validation, collapsed same-row note handling, watchdog qualification, and feature-Off recovery follow the locked rules without changing stored notes or Clip length.
- [x] Focused tests, formatting, affected ARM compilation, configured host tests, adversarial review, and one uninterrupted logged Release build pass.
- [ ] A physical Deluge and VCV Rack test confirms independent and shared clocks, irregular pulses, MIDI output, Stop, Reset, Panic, reconnect, UI feedback, and save/reload before device verification is claimed.

## Blockers

- No design blocker. `ns/bug-song-load-01` now provides the non-destructive compatibility preflight that this correction depends on.

## Artifacts

- **A1: External Step MIDI Clip plan.**
	- **Type:** Product and implementation plan.
	- **State:** Software-complete; physical and compatibility checks pending.
	- **Contents:** Restricted behavior contract, source boundary, safety rules, implementation sequence, and validation matrix.
	- **Location:** This plan.
	- **SCM surface:** Local Jujutsu change `plloqs`, commit `2e7300`, bookmark `nl/patch-midi-sequencer-01`.
- **A2: Deluge clock and VCV signaling audit.**
	- **Type:** Read-only source and protocol investigation.
	- **State:** Complete.
	- **Contents:** Verified the single Song clock, channel-less standard MIDI Clock, available learned-MIDI identity, Clip-level input dispatch, and the restricted note-pulse design seam.
	- **Location:** This plan's source entry points and worker reports.
	- **SCM surface:** Local Community Firmware checkout; no source state changed.
- **A3: External Step repository contract.**
	- **Type:** Local checkout guidance and self-contained deviation contract.
	- **State:** Complete.
	- **Contents:** Revision ownership, complete observable behavior, compatibility boundaries, edge cases, and verification requirements.
	- **Location:** `AGENTS.local.md` and deviation 23 in `DEVIATIONS.md`.
	- **SCM surface:** `DEVIATIONS.md` is tracked in Jujutsu change `plloqs`; `AGENTS.local.md` remains checkout-local guidance.
- **A4: Release artifacts.**
	- **Type:** Built firmware and link-analysis outputs.
	- **State:** Complete for the validated software revision.
	- **A4.1 BIN:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.bin`; 1,734,552 bytes; SHA-256 `d07f265433ae1a6527635df3af3a5081bab52ddce953311f96109efb2f8483b5`.
	- **A4.2 ELF:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.elf`; 65,437,756 bytes; SHA-256 `8f5dbf5bee6aaea7cee19feeb187ac45ddaa083a1ec4b57d2d3e0416545c1ab6`.
	- **A4.3 HEX:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.hex`; 4,878,758 bytes; SHA-256 `3df8ff35fcecfeb64b7570e74d3f0679e8005c545c11c081c58b18149f8ff5bc`.
	- **A4.4 MAP:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.map`; 21,270,658 bytes; SHA-256 `6fef068460fc3886141abab502c83d44ba6a7beba4fc8c6c8e277b864decdfd4`.
	- **A4.5 NMDUMP:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.nmdump`; 750,414 bytes; SHA-256 `e62354fd9f9faaf0796b7ee25e102c3ede853446d84cd216da7e6e6447a0b838`.
- **A5: Final validation logs.**
	- **Type:** Configure, host-build, test, and Release-build evidence.
	- **State:** Complete.
	- **A5.1 Host configure:** `/private/tmp/deluge-p45-host-configure-20260827T033218Z.log`.
	- **A5.2 Host build:** `/private/tmp/deluge-p45-host-build-20260827T033218Z.log`.
	- **A5.3 CTest:** `/private/tmp/deluge-p45-host-ctest-20260827T033218Z.log`.
	- **A5.4 Release:** `/private/tmp/deluge-p45-external-step-release-20260827T033618Z.log`.

## Source entry points

- **S1:** `src/deluge/io/midi/midi_engine.cpp` currently forwards accepted MIDI Clock to one global playback handler and owns early MIDI message routing.
- **S2:** `src/deluge/io/midi/learned_midi.h` documents the wildcard and MPE behavior P45 must not inherit. P45 uses a dedicated exact raw-channel binding type.
- **S3:** `src/deluge/playback/playback_handler.cpp` and `.h` own one global tick stream, learned Clip commands, transport cleanup, and the shared next-event deadline.
- **S4:** `src/deluge/playback/mode/session.cpp` advances every active Clip from the common Song increment and must exclude External Step Clips from that path.
- **S5:** `src/deluge/model/clip/instrument_clip.cpp` and `.h` own MIDI Clip note processing, local position, loop behavior, and Clip persistence.
- **S6:** `src/deluge/model/instrument/midi_instrument.cpp` owns the established channelized MIDI note and CC output path that must remain authoritative.
- **S7:** Community Feature menus, MIDI Clip settings, localization, project serialization, and `tests/spec/` need a focused source map before edits.
- **S8:** `src/deluge/model/settings/runtime_feature_settings.*` and the runtime-feature menu register the default-Off feature and own disable cleanup.
- **S9:** `src/deluge/gui/ui/menus.cpp` and Sound Editor menu learning own the MIDI Clip `CLOCK` submenu and typed Step/Reset capture.
- **S10:** `src/deluge/model/song/song.cpp`, `src/deluge/storage/Deserializer.cpp`, `src/deluge/util/firmware_version.h.in`, and `CMakeLists.txt` own the conditional downgrade guard.

## Decisions

- **Decision:** Use channelized Note, CC, or Program Change edges instead of MIDI Clock.
	- **Rationale:** Standard MIDI Clock has no channel. These Channel Voice messages preserve device, channel, message type, and number over one connection and let the VCV patch choose an appropriate control transport.
- **Decision:** Add a clock mode to the existing MIDI Instrument Clip rather than a duplicate C++ Clip class.
	- **Rationale:** Editing, output routing, probability, iterance, persistence, and display behavior can remain shared; only advancement changes.
- **Decision:** Keep the first feature Session-only and MIDI-Out-only.
	- **Rationale:** Audio, Arrangement, recording, arpeggiators, and smooth timing depend directly on the Song-wide scheduler and would turn the prototype into a clock-engine rewrite.
- **Decision:** Allow an exact Step binding to fan out to several Clips.
	- **Rationale:** This gives VCV a simple clock-domain grouping model without adding another domain object.
- **Decision:** Never silently fall back to the Song clock.
	- **Rationale:** Unexpected note output is worse than a visibly waiting Clip when the feature, binding, or source is unavailable.
- **Decision:** Keep this in component revision group `midi-sequencer`.
	- **Rationale:** The change spans MIDI Clip sequencing and input dispatch; it is not another incoming-performance feature in the existing `midi-input` sustain revision.
- **Decision:** Use a dedicated External Step binding instead of extending `LearnedMIDI`.
	- **Rationale:** The established type can wildcard cable identity, converts physical channels into MPE zones, and lacks a safe generic Program Change file form. P45 requires exact cable, raw channel, type, and number identity.
- **Decision:** Put External Step in the existing MIDI Clip Settings `CLOCK` submenu.
	- **Rationale:** Clock source is Clip behavior, not another Output type. This keeps ordinary MIDI Clip creation and routing intact and gives persisted Clips a recovery path while the feature is Off.
- **Decision:** Restrict the prototype to forward Clips whose master loop length is divisible by the saved step, without independent NoteRow timing.
	- **Rationale:** Reverse, Ping-Pong, a partial final step, and independent row phase each require another timing model. Visible validation is safer than silently rewriting notes or inventing timing.
- **Decision:** Qualify the watchdog only for stable 25 ms through 4 s pulse intervals.
	- **Rationale:** Regular clocks receive stuck-note protection after four missed intervals, while slower or intentionally irregular/manual stepping is not falsely declared disconnected.
- **Decision:** Use local save-schema sentinel 1 instead of a numeric Community version bump.
	- **Rationale:** A numeric version can later collide with unpatched upstream firmware. The unknown sentinel makes unpatched firmware refuse the file, while patched firmware can explicitly accept supported local schemas.

## Workers

- `/root`: completed the P45 compatibility correction, Jujutsu topology, final review integration, and operator communication; status: active only for physical follow-up.
- `/root/preview_review`: implemented the local save-schema 1 correction; focused compatibility, External Step, and Song-preflight tests, all 20 configured host tests, formatting, and three affected ARM Release objects passed; status: completed.
- `/root/owner_lookup`: completed independent review and found two accepted P45 malformed-input issues plus one Song-preflight issue owned by `ns/bug-song-load-01`; status: completed.
- `/root/deluge_clock_arch`: completed External Step model/runtime behavior, persistence, timing-edit and Output-conversion cleanup, optional Reset independence, and the menu-spec harness repair; status: completed.
- `/root/independent_clip_design`: completed the integrated reviews, Community Feature, Settings UI, compatibility guard, exact raw-channel handling, Learn priority, Reset independence, and conflict-transition cleanup; status: completed.
- `/root/midi_vcv_clock`: completed the exact Note/CC/Program Change matcher, input dispatch and consumption, priority and conflict checks, disconnect cleanup, held-edge rearming, and focused input tests; status: completed.

## Implementation Steps

- [x] Audit the existing Deluge clock path, learned MIDI identity, Session advancement, and VCV signaling options.
- [x] Define the restricted product model and provisional safety contract before source changes.
- [x] Receive operator authorization and activate P45 without changing the recorded owner identity.
- [x] Confirm pulse-loss timeout bounds, content constraints, downgrade safety, the UI surface, and the exact MIDI Learn conflict rule.
- [x] Update standing owner and checkout guidance, create the self-contained deviation contract, and begin the isolated Jujutsu change planned as `nl/patch-midi-sequencer-01`. No Git used.
- [x] Add the default-Off Community Feature, Clip clock-mode state, Step and Reset bindings, step size, validation, and compatible save/load behavior in the first implementation pass.
- [x] Route and consume learned pulse edges while preserving MIDI Learn priority and fan-out to all matching active Clips in the first implementation pass.
- [x] Add pulse-driven Instrument Clip advancement, exclude those Clips from Song ticks, and preserve deterministic notes, note-offs, loop wrap, probability, iterance, and stepped CC automation in the first implementation pass.
- [x] Integrate launch, Stop, Reset, Panic, Clip switching, disconnect, and bounded clock-loss cleanup without heap allocation on the MIDI event path in the first implementation pass.
- [x] Add the creation/settings UI, learn controls, `WAIT` and conflict feedback, pulse-step display, and pulse-driven playhead updates in the first implementation pass.
- [x] Add focused behavior and persistence checks, then run adversarial source review and repair accepted findings without widening scope.
- [x] Replace the ineffective `c1.3.1` downgrade marker with local save-schema 1 in this origin revision, then re-run focused compatibility tests, affected ARM compilation, and configured host tests.
- [x] Add the self-contained `DEVIATIONS.md` contract in the feature revision, run formatting, affected ARM compilation, configured host tests, and one logged Release build for the prior software state.
- [ ] Leave flashing and physical Deluge/VCV validation to the operator.

## Work Log

- [x] [[2026-08-30]] - Began correcting the older-firmware guard in its origin revision.
	- **Evidence:** Unpatched Community firmware at the same numeric version can accept `c1.3.1` and ignore External Step fields. `ns/bug-song-load-01` now rejects unsupported compatibility markers before mutating the loaded Song.
	- **Effect:** P45 will own `nl-save-schema-1`; P48 may extend the same mechanism to schema 2 without adding a competing version system.
	- **Next:** Implement and review schema 1, then revalidate P45 without running another full Release build yet.
- [x] [[2026-08-30]] - Implemented and validated local save schema 1.
	- **Evidence:** XML and JSON use one strict evaluator; `nl-save-schema-0` and `-1` load, while future, empty, malformed, leading-zero, trailing-garbage, and overflow sentinels refuse even when numeric-version checks are ignored. External Step persists schema 1 from saved Clip state, including while the runtime feature is Off. Focused tests passed 4, 5, and 7 cases; the configured host suite passed 20 of 20; formatting and three affected ARM Release objects passed.
	- **Effect:** Unpatched firmware sees an unknown marker and refuses the file. Ordinary official and Community version comparisons retain their existing path. P48 can extend the same typed maximum to schema 2.
	- **Next:** Complete independent review before creating the P48 tip.
- [ ] [[2026-08-30]] - Accepted three compatibility review findings.
	- **Evidence:** An empty ordinary firmware value can reach an out-of-bounds parser read; malformed `nl-save-schema` spellings can follow the numeric ignore path; and a compatibility marker placed after another root field can evade preflight until destructive loading begins.
	- **Effect:** The first two repairs remain in P45. The reordered-root repair returns to `ns/bug-song-load-01`. P48 remains blocked until both revisions pass review.
	- **Next:** Repair and revalidate both origin revisions, then repeat independent review.
- [x] [[2026-08-30]] - Closed the compatibility review loop.
	- **Evidence:** The generic Song-load revision now handles reordered roots, empty firmware values, and quote-aware JSON skipping. The P45 revision rejects empty, malformed, truncated, lookalike, future, and noncanonical local markers in both ignore modes. Focused suites, all configured host tests, formatting, and affected ARM objects passed; final independent review found no P1 or P2 issue.
	- **Effect:** `ns/bug-song-load-01` is `wxsvvosv` at `da03c661`; `nl/patch-midi-sequencer-01` is `plloqspk` at `61315dd4`. P48 can safely extend local schema 1 to schema 2.
	- **Next:** Physical compatibility behavior remains operator-owned.

- [x] [[2026-08-26]]@18:53 - Created the externally stepped MIDI Clip plan after the operator chose the separate step-Clip model.
	- **Evidence:** Source review found one Song-wide clock and one shared next-event scheduler; protocol review confirmed standard MIDI Clock cannot be separated by channel; existing learned MIDI state can distinguish input device, channel, and note.
	- **Effect:** P45 now specifies a bounded Session MIDI-Out prototype driven by learned Note edges. No firmware source, Jujutsu revision, build, flash, or Git operation occurred.
	- **Next:** Confirm the provisional safety choices, then activate the plan and create `nl/patch-midi-sequencer-01`.
- [x] [[2026-08-26]]@18:59 - Activated P45 after the operator approved implementation.
	- **Evidence:** The operator said “lets do this” after reviewing the P45 design summary and its provisional transport and safety choices.
	- **Effect:** P45 is active. Runtime, MIDI-input, and UI/persistence workers have distinct read-only source-map assignments; source edits remain gated on their combined design review.
	- **Next:** Lock the remaining implementation gates, update repository guidance, and create the isolated Jujutsu feature change.
- [x] [[2026-08-26]]@19:04 - Made Reset a first-class external control before source work.
	- **Evidence:** The operator required a Reset signal in addition to Advance.
	- **Effect:** P45 must learn, persist, consume, fan out, display, and test separate Step and Reset Note inputs. Reset cuts owned notes and returns to pre-step-zero without emitting a step, including while global transport is stopped.
	- **Next:** Include Reset throughout the runtime, MIDI-input, UI, persistence, and physical VCV test designs.
- [x] [[2026-08-26]]@19:05 - Expanded both control roles to Note, CC, and Program Change bindings.
	- **Evidence:** The operator explicitly required CC and Program Change in addition to Note.
	- **Effect:** Each Step or Reset binding includes device/cable, channel, message type, and number. Note uses held-edge rearming, CC uses the 64 threshold, and each matching Program Change is a discrete edge.
	- **Next:** Verify the existing learned-message representation, conflict rules, persistence, and dispatch paths for all three message types before source edits.
- [x] [[2026-08-26]]@19:16 - Completed the three source maps and locked the narrow implementation gates.
	- **Evidence:** Runtime review traced every Session tick/resync/stop seam; input review traced Note, CC, and Program Change precedence and Thru behavior; UI/persistence review verified the Settings, serializer, device-reference, and compatibility-version paths.
	- **Effect:** P45 will use a dedicated exact binding, local pulse runtime, existing MIDI Clip Settings, forward/divisible content validation, a stable-cadence watchdog, and a conditional version guard. `LearnedMIDI`, a new output type, and a second continuous scheduler are rejected.
	- **Next:** Create the isolated Jujutsu feature change, add the standing repository contract, and implement the first bounded pass.
- [x] [[2026-08-26]]@19:19 - Began the isolated feature change with guidance and deviation 23 before source edits.
	- **Evidence:** Jujutsu change `plloqspkvmmqysvtrpxlkvrnssssyzut` is directly above `nl/patch-kb-drums-01`; `DEVIATIONS.md` now inventories 23 deviations and defines the complete External Step contract; `AGENTS.local.md` records the `midi-sequencer` revision group and implementation boundaries.
	- **Effect:** Runtime, input, and UI workers have one shared contract and disjoint initial file ownership. The change contains no firmware source yet.
	- **Next:** Complete the three first-pass slices, integrate once, and run focused host checks before the adversarial review.
- [x] [[2026-08-26]]@19:48 - Integrated the three disjoint first-pass slices and froze source edits for combined review.
	- **Evidence:** Jujutsu snapshots 29 tracked files with 2,494 insertions and 247 deletions in change `plloqspkvmmqysvtrpxlkvrnssssyzut`. Each worker passed pinned formatting or focused formatting, affected ARM syntax checks, and host syntax checks for its new spec.
	- **Effect:** The change now contains the local Clip runtime, typed Note/CC/Program Change input path, Reset-before-Step dispatch, Settings UI, persistence, feature flag, compatibility guard, and three focused specs. No linked host spec, full suite, Release build, bookmark, flash, Git command, or physical test has occurred.
	- **Next:** Complete a source-wide adversarial review, repair accepted findings in this same change, then reconfigure and execute the combined host and ARM validation gate.
- [x] [[2026-08-26]]@20:38 - Completed the P45 software implementation and final validation gate.
	- **Evidence:** The integrated adversarial review passed after accepted repairs. Two final latch fixes ensure that Stop or leaving Session resets and rearms every configured External Step Clip, including inactive Clips that observed a held Note or CC, and that every higher-priority MIDI-command conflict transition rearms the affected Note or CC Step or Reset latch; a newly conflicting Step cuts owned notes once while preserving phase. The formatter covered 31 files, a clean host configure and build passed, CTest passed 18 of 18, and the uninterrupted Release build passed 504 of 504 steps in 100.69 seconds with only the existing RWX load-segment warning.
	- **Effect:** Jujutsu change `plloqs` is commit `2e7300` with bookmark `nl/patch-midi-sequencer-01`. Note, CC, and Program Change are implemented for Step and optional Reset. The validated artifacts and logs are recorded as A4 and A5. No Git command, flash, installation, publication, or upstream submission occurred.
	- **Next:** Verify physical Deluge and VCV Rack behavior, project save/reload, and older-firmware refusal.

## Validation

- **Command or action:** Read-only source trace, MIDI protocol check, VCV Core capability check, and adversarial product-contract review.
	- **Expected result:** A bounded implementation path exists that provides genuinely independent Clip stepping without pretending MIDI Clock has channels or replacing the complete playback engine.
	- **Observed result:** Passed for design and implementation. The integrated source received an adversarial review, accepted repairs, and a final review pass.
	- **Failure signals:** Song ticks advance an External Step Clip; one shared pulse reaches only the first Clip; pulse messages leak into audition or recording; Stop or clock loss leaves a note sounding; an unavailable mode silently falls back to Song time; or old firmware plays the Clip unexpectedly.
	- **Interpretation:** No software blocker remains. Physical timing, control behavior, save/reload, and older-firmware refusal still need device evidence.
- **Command or action:** Apply the pinned formatter to the P45 source and test surface.
	- **Expected result:** Every touched source and test file follows the repository formatter without unrelated rewrites.
	- **Observed result:** Passed for 31 files.
	- **Failure signals:** A formatter error, a remaining diff on recheck, or files outside the P45 surface changing.
	- **Interpretation:** The final 31-file P45 source surface passed formatting.
- **Command or action:** Clean host configure, host build, and full configured CTest run.
	- **Expected result:** The clean host tree discovers, compiles, links, and executes the complete configured suite, including the P45 specs.
	- **Observed result:** Passed. Configure and build completed cleanly; CTest passed 18 of 18 targets. A5.1 through A5.3 contain the logs.
	- **Failure signals:** Missing P45 specs, generated-header leakage into host tests, a compile or link failure, or any failed CTest target.
	- **Interpretation:** The complete configured host suite passes from a clean configure.
- **Command or action:** One uninterrupted Release build of the integrated revision.
	- **Expected result:** All Release steps complete once, the firmware artifacts are regenerated, and no new warning indicates a P45-specific problem.
	- **Observed result:** Passed 504 of 504 steps in 100.69 seconds. The only warning was the existing RWX load-segment warning. A5.4 contains the log; A4 records all five outputs, byte sizes, and SHA-256 hashes.
	- **Failure signals:** A restarted or partial build, failed step, missing output, hash or size mismatch, or a warning beyond the known RWX segment warning.
	- **Interpretation:** The ARM Release image and analysis outputs were produced successfully for commit `2e7300`.

## Unfinished Work

- [ ] Verify independent and shared Step and Reset behavior, irregular clocks, Stop, Panic, reconnect, UI feedback, and MIDI output on a physical Deluge with VCV Rack.
- [ ] Verify mode, bindings, and step size across project save and reload on the Deluge.
- [ ] Verify that older firmware refuses the incompatible saved project instead of running External Step Clips from the Song clock.
