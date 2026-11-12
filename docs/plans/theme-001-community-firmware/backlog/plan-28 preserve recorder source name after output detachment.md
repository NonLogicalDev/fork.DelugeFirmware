---
description: "Prevent a detached specific-output recorder from using an uninitialized source name when it creates its file."
tags:
  - note-type/project
  - status/seedling
  - domain/learning
  - deluge
  - firmware
---

# Preserve recorder source name after Output detachment

## Purpose

Prevent a timing-dependent bad filename pointer or crash when a `SPECIFIC_OUTPUT` recorder finishes and detaches from its `Output` before its first card routine creates the recording file. This follow-up came from P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md) on [[2026-08-24]].

## Scope

### In scope

- Keep a valid recorder-owned source label, or choose a defined fallback, after `Output::removeRecorder()` clears `outputRecordingFrom`.
- Cover the finish-before-first-`cardRoutine()` ordering directly.
- Keep the pre-existing correction in its own `ns/bug-sample-recorder-01` revision.

### Out of scope

- Redesigning recorder ownership, changing recording folders, or retaining a dereferenceable `Output*` after detachment.

## Completion Criteria

- [ ] Every `SPECIFIC_OUTPUT` filename path receives a valid source name after Output detachment.
- [ ] A focused test covers finish-before-first-card-routine ordering and fails on the current behavior.
- [ ] The affected ARM objects, host tests, and Release firmware build pass in `ns/bug-sample-recorder-01`.
- [ ] GCC `-fanalyzer` no longer reports the uninitialized `name` path.

## Blockers

None.

## Artifacts

- **Artifact:** P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **Type:** Source-reachability evidence.
	- **State:** Completed.
	- **Contents:** GCC diagnostic and the confirmed detach-before-file-creation path.
	- **Location:** P31 and `/tmp/deluge-static-analysis-ab821z/gcc-analyzer-audio-looper-sample-recorder.log`.
	- **SCM surface:** No firmware revision exists for this follow-up yet.

## Decisions

- **Decision:** Do not solve the lifetime problem by keeping or dereferencing the detached `Output*`.
	- **Rationale:** The recorder needs only stable naming data; Output detachment must continue to make the ownership pointer unusable.
- **Decision:** Keep this pre-existing defect separate from `ns/bug-audio-looper-01`.
	- **Rationale:** Upstream already detaches `outputRecordingFrom` before a possible first card routine. The Audio Looper repair changed recorder identity checks but did not create the missing-name path.

## Implementation Steps

- [ ] Choose a recorder-owned source-label or defined fallback contract.
- [ ] Add the focused lifecycle regression.
- [ ] Implement the smallest ownership-safe fix in `ns/bug-sample-recorder-01`.
- [ ] Run analyzer, host, ARM-object, and Release validation.

## Work Log

- [x] [[2026-08-24]]@17:29 - Opened the focused follow-up from P31.
	- **Evidence:** `finishCapturing()` detaches the finished recorder, `removeFromOutput()` clears `outputRecordingFrom`, and a first later `cardRoutine()` can reach filename generation with `mode == SPECIFIC_OUTPUT` and an uninitialized local `name`.
	- **Effect:** The timing-dependent crash risk is isolated from the completed analyzer audit and the Audio Looper behavior revision.
	- **Next:** Add a deterministic lifecycle test before changing firmware.

## Validation

- **Command or action:** Focused finish-before-first-card-routine test.
	- **Expected result:** Filename generation receives a valid stable label after detachment.
	- **Observed result:** Not yet run.
	- **Failure signals:** Uninitialized reads, stale Output dereferences, invalid paths, or changed normal recorder names.
	- **Interpretation:** Passing will prove the identified ordering is safe; physical recording remains separate validation.

## Unfinished Work

- [ ] Implement and validate the focused fix.
