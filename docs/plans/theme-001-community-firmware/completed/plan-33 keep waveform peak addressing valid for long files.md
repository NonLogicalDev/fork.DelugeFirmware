---
description: "Keep shared waveform peak collection valid across long sample and file positions."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Keep waveform peak addressing valid for long files

## Purpose

Prevent the shared waveform peak collector from wrapping valid long-sample positions or file-byte offsets through signed 32-bit arithmetic. The defect predates the local OLED feature and must remain independently reviewable in `ns/bug-waveform-01`. This plan was separated from P30 [Add efficient OLED waveform displays](../active/plan-26%20add%20efficient%20OLED%20waveform%20displays.md) on [[2026-08-24]].

## Scope

### In scope

- Preserve valid sample positions at and beyond the signed 32-bit boundary throughout waveform-column range calculation.
- Preserve valid byte positions through the recorder's 4 GiB file domain and reject arithmetic or cluster-index values that the downstream representation cannot accept.
- Keep ordinary short-file, recording, forward, reversed, partial-load, and beyond-waveform peak behavior unchanged.
- Place the correction and its generic boundary regression in the separate component revision `ns/bug-waveform-01`.

### Out of scope

- OLED cache geometry, OLED drawing, marker presentation, Slicer behavior, audio playback, file-format expansion, or changing the recorder's maximum file size.

## Current State

- Status: completed. The generic long-address correction and its focused regression are isolated and validated in `ns/bug-waveform-01`.
- Next action: none for P37.

## Completion Criteria

- [x] A valid waveform viewport never narrows its sample bounds through signed 32-bit arithmetic before range validation.
- [x] Valid byte offsets across 2 GiB and through the recorder's 4 GiB domain remain non-negative and map to the intended cluster and byte-within-cluster.
- [x] Unsupported arithmetic, cluster sizes, or downstream cluster indexes fail safely without array, sample, or storage access outside their valid domain.
- [x] Focused generic tests cover positions immediately below, at, and above the signed 32-bit byte boundary, the 4 GiB boundary, positions beyond `INT32_MAX` samples, negative positions, and arithmetic overflow.
- [x] Formatting, the affected ARM object, configured host suite, static analysis, and a local Release build pass after the correction is isolated in `ns/bug-waveform-01`.
- [x] P30 retains only OLED-specific cache, rendering, and integration behavior.

## Blockers

None.

## Artifacts

- **Artifact:** Long-address source and boundary evidence.
	- **Type:** Combined implementation and focused validation evidence.
	- **State:** Complete and independently validated after separation.
	- **Contents:** Signed sample positions, unsigned byte positions, cluster derivation checks, and deterministic 2 GiB and 4 GiB boundary cases.
	- **Location:** P30 [Add efficient OLED waveform displays](../active/plan-26%20add%20efficient%20OLED%20waveform%20displays.md) and P31 [Run static analysis on crash-sensitive firmware paths](plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **SCM surface:** Jujutsu change `rrrrtvstzpvkmlsswkwqqtomqmxklmnr`, commit `026a809d9b8fd167badc979422413c613d182d6f`, bookmark `ns/bug-waveform-01`.

## Decisions

- **Decision:** Treat the address-width defect as pre-existing shared waveform behavior, not as part of the OLED feature.
	- **Rationale:** Upstream peak collection accepts 64-bit viewport inputs but narrows sample and byte positions internally. OLED work exposed the defect; it did not create it.
- **Decision:** Keep this revision independent from the reversed-range correction in P38.
	- **Rationale:** Address width and half-open reversal have different causes, invariants, and regression cases even though both affect the shared waveform renderer.

## Implementation Steps

- [x] Separate the generic sample, byte, and cluster address helpers and collector correction from `nl/patch-oled-01` into `ns/bug-waveform-01`.
- [x] Keep the long-address regression with the bug revision rather than only in an OLED-specific test surface.
- [x] Recheck all narrowing points and the recorder's actual 4 GiB limit.
- [x] Format, analyze, run focused and configured host tests, compile the affected ARM object, and complete a local Release build.
- [x] Update P30 and owner state with the final separated revision evidence.

## Work Log

- [x] [[2026-08-24]] - Confirmed the upstream long-address defect and assigned its revision.
	- **Evidence:** `WaveformRenderer::findPeaksPerCol()` receives 64-bit viewport inputs but used signed 32-bit sample and byte bounds. At 44.1 kHz and 24-bit depth, signed byte arithmetic reaches 2 GiB at about 2:15:16 for stereo and 4:30:32 for mono, while native recording permits files through 4 GiB.
	- **Effect:** The correction is owned by `ns/bug-waveform-01`; OLED may depend on it but may not contain it.
	- **Next:** Separate the existing combined implementation and validate the resulting revision independently.
- [x] [[2026-08-24]] - Completed and validated the separated long-address bug revision.
	- **Evidence:** `waveform_large_address_spec` passed its two focused cases. The affected ARM `waveform_renderer.cpp` object compiled. The configured CTest suite passed 10 of 10 targets, and the clean 506-step Release build completed successfully.
	- **Effect:** `ns/bug-waveform-01` now owns only the pre-existing long-file address repair and its generic regression. `nl/patch-oled-01` no longer owns this defect.
	- **Next:** None.

## Validation

- **Command or action:** Focused long-address regression, ARM object compile, configured host suite, static analysis, and clean Release build after revision separation.
	- **Expected result:** Positions across 2 GiB and 4 GiB remain valid without signed wrap, while invalid inputs are rejected.
	- **Observed result:** Passed. `waveform_large_address_spec` passed two focused cases, the affected ARM `waveform_renderer.cpp` object compiled, and ARM GCC `-fanalyzer` produced zero diagnostics for the post-split renderer. The full configured CTest suite passed 10 of 10 targets, and `/tmp/deluge-oled-postsplit-build.log` records a clean 506-step Release build. LLVM reproduced only previously reviewed bounded conversion and API-shape warnings, with no new reachable finding.
	- **Failure signals:** Negative wrapped byte positions, truncated sample positions, invalid cluster narrowing, a test that depends on OLED-only state, or failure after the revision is separated.
	- **Interpretation:** The dedicated bug revision has focused, target-object, analyzer, host-suite, and full-build evidence independent of OLED revision ownership.

## Unfinished Work

None.
