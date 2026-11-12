---
description: "Prevent low-memory MIDI sustain note-off handling from throwing out of the input path."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Make MIDI sustain deferral allocation-safe

## Purpose

Prevent incoming sustain-pedal note-off handling from terminating or resetting under low memory when the deferred-note map allocates a node and throws `BAD_ALLOC`. The local sustain feature introduced this allocation path, so the correction must replace that implementation inside `nl/patch-midi-input-01`. This follow-up came from P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md) on [[2026-08-24]].

## Current State

- Status: software-complete in `nl/patch-midi-input-01`, Jujutsu change `xookvmtnplsp`, commit `d194d533e392`. Deferred note-off state is now a fixed 128-byte table indexed by MIDI pitch and cannot allocate on the live input path.
- Next action: physically confirm CC64 sustain, same-note retrigger, feature disable, Stop, and Panic on the Deluge. No further source work is known.

## Workers

- `/root`: completed the fixed-state design, source integration, focused tests, adversarial lifecycle review, Jujutsu fold, combined validation, and owner records.

## Product integration

- **Existing model:** Incoming internal-Synth note-offs are deferred while CC64 is down, stored with release velocity, cancelled by a same-pitch retrigger, and released on pedal-up or cleanup. The current map expresses the behavior correctly but allocates on the live note-off path.
- **Required model:** Keep exactly the same note lifecycle in a fixed domain indexed by MIDI pitch 0 through 127. Deferral, replacement, lookup, removal, iteration, and cleanup must not allocate.
- **Smallest repair:** Introduce one small allocation-free sustain-state type with focused host coverage, then replace only the feature-owned map operations. Do not change `notesAuditioned`, general MIDI input, recording, or Synth voice behavior.

## Scope

### In scope

- Make the bounded 128-note sustain deferral state allocation-free, or release the current note safely if allocation fails.
- Preserve pedal thresholds, retrigger replacement, recorded physical note-off timing, feature-disable cleanup, stop, and Panic behavior.
- Fold the correction into the established `nl/patch-midi-input-01` component revision.

### Out of scope

- Changing MIDI Learn, external MIDI thru, pedal polarity, or sustain behavior outside internal Synth note lifetime.

## Completion Criteria

- [x] Pedal-held note-off handling cannot throw because it needs a new deferred-note allocation.
- [x] All 128 MIDI notes have deterministic defer, retrigger, release, disable, stop, and Panic behavior.
- [x] A focused allocation-free-state test proves the input path remains independent of the firmware allocator.
- [x] The affected ARM objects, configured host suite, and Release firmware build pass after the fix is folded into `nl/patch-midi-input-01`.

## Blockers

None.

## Artifacts

- **Artifact:** P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **Type:** Allocation-path source audit.
	- **State:** Completed.
	- **Contents:** Deferred-note insertion, custom allocator failure behavior, and uncaught MIDI input path.
	- **Location:** P31 and the current `MelodicInstrument` sustain implementation.
	- **SCM surface:** The future correction belongs in `nl/patch-midi-input-01`; no source change was made by this investigation.
- **Artifact:** Allocation-safe MIDI sustain implementation and focused specification.
	- **Type:** Firmware source, fixed-state unit specification, and local Jujutsu revision.
	- **State:** Software-complete; physical confirmation remains.
	- **Contents:** One 128-byte deferred-note table, `MelodicInstrument` integration, all-pitch/retrigger/release/cleanup coverage, and the existing self-contained sustain contract in `DEVIATIONS.md`.
	- **Location:** `src/deluge/model/instrument/midi_sustain_deferred_notes.h`, `src/deluge/model/instrument/melodic_instrument.{h,cpp}`, and `tests/spec/midi_sustain_deferred_notes_spec.cpp` in `nl/patch-midi-input-01`.
	- **SCM surface:** Jujutsu change `xookvmtnplsp`, commit `d194d533e392`; no Git command was used.
- **Artifact:** Release firmware image.
	- **Type:** Local build output.
	- **State:** Built and hashed; not flashed or published.
	- **Contents:** `build/Release/deluge.bin`, 1,717,560 bytes, SHA-256 `4a325f672ca423a641a8e02ebf4b1f5384e83741f6b97e9e7fe469ff98e64939`.
	- **Location:** Firmware checkout and `/private/tmp/deluge-p36-release-20260825T023430-0700.log`.
	- **SCM surface:** Derived from the final rebased local stack whose tip is `nl/patch-sample-editor-02` commit `919731bfd645`.

## Decisions

- **Decision:** Prefer fixed state indexed by the 128-note MIDI domain.
	- **Rationale:** The key range is small and closed. Fixed storage removes event-path allocation, exception handling, tree overhead, and low-memory ambiguity while keeping constant-time access.
- **Decision:** Fold the repair into `nl/patch-midi-input-01`.
	- **Rationale:** Sustain deferral introduced the allocation path. The local stack should retain one final implementation of that feature rather than a later cancellation patch.

## Implementation Steps

- [x] Specify fixed per-note occupancy and stored release velocity.
- [x] Add full-domain defer, retrigger, release, and cleanup tests, including allocator independence.
- [x] Replace event-path dynamic insertion while preserving external behavior.
- [x] Fold, format, analyze, test, build, and independently review the corrected MIDI-input revision.

## Work Log

- [x] [[2026-08-24]]@17:38 - Isolated the sustain allocation-failure path.
	- **Evidence:** Pedal-held note-off uses map indexing, the custom allocator throws `BAD_ALLOC` when fast memory is exhausted, and the MIDI input path has no catch or fallback release.
	- **Effect:** Normal-memory behavior is coherent, but low memory can turn a note-off into an uncaught exception rather than a safe release.
	- **Next:** Replace the dynamic event-path state with a bounded representation and fold it into `nl/patch-midi-input-01`.
- [x] [[2026-08-25]]@02:31 - Resumed the local-regression repair and reverified the current source.
	- **Evidence:** `MelodicInstrument::receivedNote()` still writes pedal-held releases through `fast_map::operator[]`; note-on erases the same pitch, pedal-up iterates every deferred entry, and Stop/Panic clear the map.
	- **Effect:** The accepted behavior is already narrow and coherent. Only its storage representation and focused coverage need to change.
	- **Next:** Activate this plan, implement fixed 128-note state in a temporary top change, review it, and fold it into `nl/patch-midi-input-01`.
- [x] [[2026-08-25]]@02:39 - Replaced the allocating deferred-note map, folded the correction, and completed combined validation.
	- **Evidence:** The fixed table covers pitches 0 through 127 and stores release velocity without allocation. Same-note note-on erases its pending release; pedal-up drains each occupied slot after clearing it; feature disable releases deferred notes; Stop and Panic clear the table and pedal state; physical note-off recording and MIDI Learn safeguards remain before or outside deferral. The first full build exposed an ambiguous `deluge::midi` helper namespace through `menus.cpp`; renaming it to `deluge::midi_support` made the previously failing menu and sustain objects compile before the replacement build was started.
	- **Effect:** The low-memory exception path is gone without changing the accepted sustain lifecycle. The temporary correction revisions were squashed into `nl/patch-midi-input-01`, and ten descendants were rebased so the tip contains no unused intermediate implementation.
	- **Next:** Physically verify CC64, retrigger, disable, Stop, and Panic behavior on hardware.

## Validation

- **Command or action:** Full 128-note sustain state and low-memory-independence tests.
	- **Expected result:** Deferral and cleanup are deterministic for every note without allocation; a simulated exhausted allocator cannot affect note-off handling.
	- **Observed result:** `midi_sustain_deferred_notes_spec` passed for all 128 pitches, release velocities 0 through 127, repeated note-off replacement, retrigger erase, cleanup, and invalid pitches. Compile-time size and trivial-copy assertions prove the state is a 128-byte fixed object with no allocator-owned storage.
	- **Failure signals:** Thrown allocation errors, lost deferred notes, stuck notes, changed physical note-off recording, or incomplete Panic/disable cleanup.
	- **Interpretation:** Passing removes the low-memory crash path while preserving the established sustain contract.
- **Command or action:** Project formatting, affected ARM Release objects, configured host suite, and logged Release firmware build.
	- **Expected result:** The authoritative rewritten stack formats, compiles, links, and passes every configured host test.
	- **Observed result:** `clang-format --dry-run --Werror` passed; `melodic_instrument.cpp.obj` and `menus.cpp.obj` passed after the namespace repair; 14 of 14 CTest targets passed before and after the final build. The uninterrupted replacement build ran from [[2026-08-25]] 02:34:30 to 02:38:51 PDT, completed all 117 steps, and linked `Release/deluge.elf`. The expected existing RWX segment linker warning remained.
	- **Failure signals:** Formatter drift, compiler or linker errors, host-test failures, an interrupted process, or a stale artifact.
	- **Interpretation:** The final local stack is software-buildable; hardware behavior remains the only unverified boundary.

## Unfinished Work

- [ ] Physically confirm sustain, retrigger, feature disable, Stop, and Panic on the Deluge.
