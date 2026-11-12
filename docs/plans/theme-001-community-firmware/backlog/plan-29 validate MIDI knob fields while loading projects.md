---
description: "Prevent incomplete MIDI-knob project data from constructing a mapping with uninitialized fields."
tags:
  - note-type/project
  - status/seedling
  - domain/learning
  - deluge
  - firmware
---

# Validate MIDI knob fields while loading projects

## Purpose

Make incomplete or malformed project data deterministic instead of constructing a MIDI knob from uninitialized channel, CC, or relative-mode fields. This follow-up came from P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md) on [[2026-08-24]].

## Scope

### In scope

- Initialize MIDI-knob parse state and validate the fields required before appending a mapping.
- Preserve valid saved-project behavior and define compatibility for omitted optional fields.
- Add malformed-input coverage for each omitted required field.
- Keep the pre-existing parser correction in its own `ns/bug-midi-knobs-01` revision.

### Out of scope

- Changing MIDI Learn behavior, the on-disk schema for valid projects, or unrelated project-file parser cleanup.

## Completion Criteria

- [ ] No MIDI-knob parse path reads an uninitialized channel, CC number, or relative flag.
- [ ] Valid project data loads exactly as before.
- [ ] Focused malformed-input cases establish the chosen skip/default policy.
- [ ] The affected ARM object, host tests, and Release firmware build pass.
- [ ] The fix is isolated in `ns/bug-midi-knobs-01`.

## Blockers

None.

## Artifacts

- **Artifact:** P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **Type:** Source-reachability evidence.
	- **State:** Completed.
	- **Contents:** GCC diagnostics and the confirmed incomplete-object parse path.
	- **Location:** P31 and `/tmp/deluge-static-analysis-ab821z/gcc-analyzer-panic-mod-controllable.log`.
	- **SCM surface:** No firmware revision exists for this follow-up yet.

## Decisions

- **Decision:** Treat this as parser hardening, separate from Panic.
	- **Rationale:** The warning appears in a file touched by Panic, but the reachable path is project loading and predates the teardown change.
- **Decision:** Use the separate `midi-knobs` bug group.
	- **Rationale:** The defect predates every local feature that touched the file, so it must not be folded into Panic or another feature revision.

## Implementation Steps

- [ ] Decide and document which fields are required and which may use backward-compatible defaults.
- [ ] Add focused valid and incomplete project-data cases.
- [ ] Implement the smallest parser-only fix in `ns/bug-midi-knobs-01`.
- [ ] Run analyzer, host, ARM-object, and Release validation.

## Work Log

- [x] [[2026-08-24]]@17:29 - Opened the focused follow-up from P31.
	- **Evidence:** `ModControllableAudio::readTagFromFile()` declares `channel`, `ccNumber`, and `relative` without initial values, then appends a knob whenever `controlsParam` is valid even if any of those fields was omitted.
	- **Effect:** The low-reachability parser defect is tracked without folding an unrelated fix into Panic.
	- **Next:** Define the missing-field compatibility policy and add focused fixtures.

## Validation

- **Command or action:** Valid and incomplete MIDI-knob project parsing tests.
	- **Expected result:** Valid mappings are unchanged; incomplete mappings follow the documented policy without uninitialized reads.
	- **Observed result:** Not yet run.
	- **Failure signals:** Random mapping values, changed valid-file behavior, or silent acceptance without defined defaults.
	- **Interpretation:** Passing will prove parser determinism; physical MIDI Learn behavior is outside this fix.

## Unfinished Work

- [ ] Implement and validate the focused parser fix.
