---
description: "Remove uninitialized resampling-strength arguments from native-rate Stutter recording."
tags:
  - note-type/project
  - status/seedling
  - domain/learning
  - deluge
  - firmware
---

# Remove native Stutter uninitialized strength values

## Purpose

Remove undefined behavior from native-rate Stutter recording, where uninitialized strength values are passed by value to the delay-buffer writer even though its native branch does not use them. This follow-up came from P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md) on [[2026-08-24]].

## Scope

### In scope

- Use the native writer without evaluating resampling strengths on the native-rate branch.
- Preserve the resampled branch and Stutter playback behavior.
- Add a focused native-versus-resampled branch regression or deterministic harness.
- Keep the pre-existing Stutter correction in its own `ns/bug-stutter-01` revision.

### Out of scope

- Changing Stutter timing, rate mapping, buffer size, ping-pong behavior, or sound design.

## Completion Criteria

- [ ] Native-rate recording evaluates no uninitialized scalar and uses the established native write operation.
- [ ] Resampled recording retains its current interpolation weights.
- [ ] Focused checks cover both branches, and ARM GCC `-fanalyzer` no longer reports the path.
- [ ] The affected ARM object, configured host suite, and Release firmware build pass in `ns/bug-stutter-01`.

## Blockers

None.

## Artifacts

- **Artifact:** P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **Type:** Analyzer and source evidence.
	- **State:** Completed.
	- **Contents:** GCC uninitialized-value diagnostic and independent native-branch review.
	- **Location:** P31 and `/tmp/deluge-static-analysis-ab821z/gcc-analyzer-panic-stutterer.log`.
	- **SCM surface:** The future fix belongs in `ns/bug-stutter-01`.

## Decisions

- **Decision:** Separate native writing from resampling-strength calculation.
	- **Rationale:** Initializing unused weights would silence the symptom. Calling the established native writer expresses the actual branch contract and removes the invalid reads.
- **Decision:** Use a separate Stutter bug revision rather than folding the fix into Panic.
	- **Rationale:** The native recording path exists upstream and predates the local Panic feature, even though the analyzer encountered it while scanning Panic-adjacent audio teardown.

## Implementation Steps

- [ ] Add focused native and resampled recording coverage.
- [ ] Route native recording through the established native writer.
- [ ] Re-run ARM GCC analysis and normal build validation.
- [ ] Record the fix in `ns/bug-stutter-01` and its contract.

## Work Log

- [x] [[2026-08-24]]@17:38 - Confirmed the native Stutter undefined behavior.
	- **Evidence:** Native recording leaves both strength locals unset, then performs lvalue-to-rvalue conversions while passing them into a writer that returns before using them.
	- **Effect:** The common native path has source-level undefined behavior even when current builds appear to ignore the unused values.
	- **Next:** Encode the intended native branch directly and retain the resampled calculation only where it is used.

## Validation

- **Command or action:** Native and resampled Stutter recording harness plus ARM GCC analysis.
	- **Expected result:** Native recording takes no resampling weights; resampled recording retains the same calculated weights; the analyzer log is empty.
	- **Observed result:** Not yet run.
	- **Failure signals:** Changed Stutter audio sequence, uninitialized-value diagnostics, or shared delay-buffer regressions.
	- **Interpretation:** Passing removes the confirmed undefined behavior without broad Stutter redesign.

## Unfinished Work

- [ ] Implement and validate the focused Stutter fix.
