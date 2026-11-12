---
description: "Keep reversed single-row waveform ranges half-open and inside the display."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Correct reversed waveform row ranges

## Purpose

Prevent reversed single-row waveform rendering from producing a negative source column or an invalid empty divisor when it mirrors a half-open display range. The defect predates the local OLED feature and must remain independently reviewable in `ns/bug-waveform-02`. This plan was separated from P30 [Add efficient OLED waveform displays](../active/plan-26%20add%20efficient%20OLED%20waveform%20displays.md) on [[2026-08-24]].

## Scope

### In scope

- Mirror every valid half-open source interval within the 16-column pad waveform without changing its length.
- Keep the full range and every valid subset inside columns zero through sixteen, with an exclusive end.
- Reject invalid caller ranges before peak status or sample-column access.
- Keep a focused generic regression with the separate component revision `ns/bug-waveform-02`.

### Out of scope

- OLED contour reversal, sample playback direction, marker semantics, pad colors, sample-address width, or unrelated waveform rendering cleanup.

## Current State

- Status: completed. The generic half-open reversal correction, invalid-range guard, and focused regression are isolated and validated in `ns/bug-waveform-02`.
- Next action: none for P38.

## Completion Criteria

- [x] Reversing the full half-open range `[0,16)` yields `[0,16)` and never reaches column `-1`.
- [x] Reversing any valid subset preserves its length and mirrors it within `[0,16)`.
- [x] Invalid negative, inverted, or beyond-display ranges fail before any peak-cache indexing or divisor calculation.
- [x] Focused generic tests cover the full range, representative subsets, first and last columns, and invalid ranges.
- [x] Formatting, the affected ARM object, configured host suite, static analysis, and a local Release build pass after the correction is isolated in `ns/bug-waveform-02`.
- [x] P30 retains only OLED-specific reversal and display behavior.

## Blockers

None.

## Artifacts

- **Artifact:** Reversed-range source and regression evidence.
	- **Type:** Combined implementation and focused validation evidence.
	- **State:** Complete and independently validated after separation.
	- **Contents:** Correct half-open reversal, invalid-range rejection, and full/subset mapping cases.
	- **Location:** P30 [Add efficient OLED waveform displays](../active/plan-26%20add%20efficient%20OLED%20waveform%20displays.md) and P31 [Run static analysis on crash-sensitive firmware paths](plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **SCM surface:** Jujutsu change `ryzzlqzzlnrpzzlkxlmvplxvmtwopznu`, commit `c4bfc3d7a1157130b868aa08bfedb97c1b256c5e`, bookmark `ns/bug-waveform-02`.

## Decisions

- **Decision:** Treat the reversed-range defect as pre-existing shared waveform behavior, not as part of the OLED feature.
	- **Rationale:** Upstream reversed single-row rendering maps a full valid range to `[-1,15)`. OLED review exposed the defect; it did not create it.
- **Decision:** Keep a focused regression even though both analyzers missed the defect.
	- **Rationale:** The source path is concrete and reachable. Analyzer silence did not prove the range arithmetic safe.

## Implementation Steps

- [x] Separate the half-open reversal helper, caller correction, and invalid-range guard from `nl/patch-oled-01` into `ns/bug-waveform-02`.
- [x] Keep the full-range and subset regression with the bug revision rather than only in an OLED-specific test surface.
- [x] Recheck forward and reversed single-row callers for the same interval contract.
- [x] Format, analyze, run focused and configured host tests, compile the affected ARM object, and complete a local Release build.
- [x] Update P30 and owner state with the final separated revision evidence.

## Work Log

- [x] [[2026-08-24]] - Confirmed the upstream half-open reversal defect and assigned its revision.
	- **Evidence:** The prior mapping subtracts both endpoints from `width - 1`, so a requested `[0,16)` source interval becomes `[-1,15)`. That permits `colStatus[-1]` and makes the first derived bucket span empty.
	- **Effect:** The correction is owned by `ns/bug-waveform-02`; OLED may depend on it but may not contain it.
	- **Next:** Separate the existing combined implementation and validate the resulting revision independently.
- [x] [[2026-08-24]] - Completed and validated the separated reversed-range bug revision.
	- **Evidence:** `waveform_reversed_range_spec` passed its focused case. The affected ARM `waveform_renderer.cpp` object compiled. The configured CTest suite passed 10 of 10 targets, and the clean 506-step Release build completed successfully.
	- **Effect:** `ns/bug-waveform-02` now owns only the pre-existing half-open reversal repair and its generic regression. `nl/patch-oled-01` no longer owns this defect.
	- **Next:** None.

## Validation

- **Command or action:** Focused reversed-range regression, ARM object compile, configured host suite, static analysis, and clean Release build after revision separation.
	- **Expected result:** Full and subset ranges mirror inside the display, and invalid inputs cannot reach cache indexing.
	- **Observed result:** Passed. `waveform_reversed_range_spec` passed its focused case, the affected ARM `waveform_renderer.cpp` object compiled, and ARM GCC `-fanalyzer` produced zero diagnostics for the post-split renderer. The full configured CTest suite passed 10 of 10 targets, and `/tmp/deluge-oled-postsplit-build.log` records a clean 506-step Release build. Both analyzers missed the pre-fix defect; LLVM's post-split rerun added no new reachable finding.
	- **Failure signals:** A negative source column, changed range length, an endpoint outside zero through sixteen, an invalid divisor, or failure after the revision is separated.
	- **Interpretation:** The dedicated bug revision has focused, target-object, analyzer, host-suite, and full-build evidence independent of OLED revision ownership. The explicit regression remains necessary because analyzer cleanliness did not expose the original defect.

## Unfinished Work

None.
