---
description: "Prevent new-Kit Manual Slice confirmation from dereferencing missing generated-drum entries."
tags:
  - note-type/project
  - status/seedling
  - domain/learning
  - deluge
  - firmware
---

# Retain every generated Manual Slice drum

## Purpose

Prevent the deterministic null dereference that occurs when Manual Slice creates two or more slices in a brand-new Kit. The current local `sample-slicer` placement split introduced this regression, so the correction must replace that implementation inside `nl/patch-sample-slicer-04`. This follow-up came from P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md) on [[2026-08-24]].

## Product Integration

- Existing model: Manual Slice constructs one Sound Drum per requested range, then either places the drums into an existing Kit immediately or leaves new-Kit row assignment to the established generic path. Successful confirmation later applies each manual range to the corresponding constructed drum.
- User intent: every requested Manual slice must survive confirmation as a playable Kit row, regardless of whether the destination Kit existed before Slicer entry.
- Clean integrated model: drum construction records every successful generated drum independently of row-placement policy. Existing-Kit and new-Kit placement may diverge afterward, but both consume the same complete generated-drum sequence and confirmation never relies on an entry that placement code forgot to retain.
- Existing pieces that must remain unchanged: Region Slice creation, manual slice points, batch naming, playback-mode selection, existing-Kit anchor rules, allocation-failure cleanup, and generic new-Kit row placement.
- Architecture impact: one lifecycle correction and focused regression inside the established Slicer feature; no new feature surface, data model, or patch group.

## Current State

- Status: software-complete and folded into `nl/patch-sample-slicer-04` at `52485167b3363c2e75b43ac01156eaba5f9fa6a1`. Combined P23/P34 review, formatter, host, ARM, and Release gates pass.
- Next action: physically re-test a brand-new Kit with at least two Manual slices, an existing Kit Manual batch, and Region slicing above 64. The agent will not flash the device.

## Scope

### In scope

- Record every successfully created Manual Slice `SoundDrum*` before new-Kit and existing-Kit placement diverge.
- Keep failed allocations unset and leave Region Slice behavior unchanged.
- Fold the correction and regression into the established `nl/patch-sample-slicer-04` component revision.

### Out of scope

- Changing slice points, naming, row placement, playback mode, or the Slicer interaction model.

## Completion Criteria

- [x] Confirming two or more Manual slices in a brand-new Kit no longer reads a null generated-drum entry in the source-proven lifecycle; physical confirmation remains.
- [x] Existing-Kit Manual Slice and Region Slice retain their current placement behavior in the final reviewed source.
- [x] Focused source and analyzer review prove the corrected common post-placement assignment, strict confirmation, and complete Region exclusion. No helper-only proxy remains; a physical new-Kit Manual re-test remains required because the current host harness cannot construct the lifecycle proportionately.
- [x] The affected ARM object, configured host suite, and Release firmware build pass after the fix is folded into `nl/patch-sample-slicer-04`.

## Blockers

None.

## Workers

- `/root`: completed P34 scope, plan state, Jujutsu folding, final software validation, and user communication; physical-device validation remains.
- `/root/manual_slice_drum_audit`: completed the exact construction, storage, placement, cleanup, and confirmation audit; confirmed the null path and common post-branch repair boundary.
- `/root/manual_slice_regression_design`: completed the host-test design; rejected a broad fake UI lifecycle and specified a small policy seam with new-Kit, existing-Kit, failed-placement, and Region cases.
- `/root/manual_slice_drum_impl`: completed the corrected one-file P34 draft in temporary change `otvoouvyozts`, commit `e1b985c5a610`; formatter, UnitTests rebuild, and affected ARM object compile passed.
- `/root/manual_slice_drum_review`: completed the final re-review with no blockers; confirmed new-Kit closure, existing-Kit failure ordering, complete Region exclusion, and unchanged first-drum/confirmation behavior.

## Artifacts

- **Artifact:** P31 [Static analysis on crash-sensitive firmware paths](../completed/plan-27%20run%20static%20analysis%20on%20crash-sensitive%20firmware%20paths.md).
	- **Type:** Independent source audit.
	- **State:** Completed.
	- **Contents:** Cross-function reachability from new-Kit Slicer entry through generated drums and post-slice bound assignment.
	- **Location:** P31 and the current `Slicer::doSlice()` and Manual confirm paths.
	- **SCM surface:** The future correction belongs in `nl/patch-sample-slicer-04`; no source change was made by this investigation.

## Decisions

- **Decision:** Store the generated drum independently of row-placement policy.
	- **Rationale:** Manual bound assignment needs the drum for both new and existing Kits. Whether the drum receives a new note row or later generic assignment is a separate concern.
- **Decision:** Fold the repair into `nl/patch-sample-slicer-04`.
	- **Rationale:** That component revision introduced the split existing-Kit placement path. A separate cancellation patch would leave obsolete behavior in the local stack.
- **Decision:** Publish the generated-drum alias only after either placement branch has completed successfully.
	- **Rationale:** Publishing immediately after drum allocation would leave a stale alias when existing-Kit NoteRow allocation fails and destroys the drum. A common post-branch publication keeps construction state independent of row-placement policy without changing current cleanup semantics.
- **Decision:** Do not add a null guard to Manual confirmation.
	- **Rationale:** Skipping a missing drum would hide the crash by silently omitting a requested slice. The invariant is that every successfully generated Manual slice is retained, not that confirmation tolerates incomplete success.
- **Decision:** Reject a helper-only placement-result seam and use one direct guarded assignment at the common successful-placement boundary.
	- **Rationale:** The first draft's helper tests never entered `Slicer::doSlice()` and would still pass if production publication disappeared or moved back into the wrong branch. Worse, passing `manualSliceDrums[i]` formed an out-of-bounds reference for Region counts above 64 before the helper could reject non-Manual mode. A direct `isManualSlice` guard before the array access is smaller and excludes Region structurally. A full Slicer host fixture would require broad fake UI architecture, so focused source/analyzer review plus the operator's physical regression sequence is the proportionate proof.

## Implementation Steps

- [x] Establish a focused source/analyzer regression proof for a brand-new Kit with at least two Manual slices, and record why the current host harness cannot exercise the full lifecycle without broad test-only architecture.
- [x] Store each successfully constructed Manual Slice drum at the common successful-placement boundary.
- [x] Recheck allocation-failure cleanup and both placement modes.
- [x] Fold, format, test, build, and independently review the corrected Slicer revision.

## Work Log

- [x] [[2026-08-24]]@17:38 - Isolated the new-Kit Manual Slice confirm crash.
	- **Evidence:** The first drum is stored, later drums are stored only inside the existing-Kit branch, and the successful Manual confirm path dereferences all `numManualSlice` entries unconditionally.
	- **Effect:** A brand-new Kit with at least two Manual slices has a deterministic null entry at index one before bounds are assigned.
	- **Next:** Add the failing lifecycle regression before folding the smallest correction into `nl/patch-sample-slicer-04`.
- [x] [[2026-08-24]]@21:46 - Activated P34 ahead of the lower-probability low-memory sustain regression.
	- **Evidence:** P34 is a deterministic null dereference for an ordinary brand-new-Kit Manual Slice confirmation with at least two slices. P36 requires allocator exhaustion. Both are local-feature regressions that must be folded, but P34 has the more direct user-reachable failure mode.
	- **Effect:** P34 moves from backlog to active without widening its scope. Independent workers will reconcile the lifecycle and regression seam before any repository edit.
	- **Next:** Accept the source and test design into this plan, then implement only the lifecycle correction inside a temporary child for review before folding.
- [x] [[2026-08-24]]@21:51 - Accepted the confirmed lifecycle and focused-regression design.
	- **Evidence:** New-Kit entry clears the alias table and sets `usesExistingKit = false`. `doSlice()` records slice zero, adds every later drum to the Kit, but records later aliases only inside the existing-Kit branch. Successful confirmation then dereferences every requested alias. Region Slice never reads this table. A real host Slicer lifecycle would require broad new doubles for SampleBrowser, SoundEditor, Kit, InstrumentClip, NoteRow, ParamManager, audio, display, allocation, and global UI state.
	- **Effect:** The smallest accepted draft records each Manual Slice drum once after either placement branch succeeds and keeps confirmation strict. A dependency-free placement-result policy supplies the focused red/green proof; source review must still prove that production calls it at the shared post-branch point.
	- **Next:** Create a temporary child of the clean stack tip, implement the draft with focused tests, then send its exact diff to adversarial review before any fold.
- [x] [[2026-08-24]]@21:53 - Received physical-device confirmation that Manual slicing crashes the Deluge.
	- **Evidence:** The operator reports that Manual slicing no longer works and crashes the device. This is consistent with the source-proven new-Kit confirmation path, where any successful Manual batch of two or more slices dereferences the missing second generated-drum alias.
	- **Effect:** P34 is release-blocking for the local firmware stack. Continue the exact lifecycle repair and full software gate before asking for a physical re-test; do not broaden the patch or mask the missing slice with a null guard.
	- **Next:** Finish the draft, adversarially review it, fold it into `nl/patch-sample-slicer-04`, rebuild, and return a new firmware artifact for the operator's confirmation.
- [x] [[2026-08-24]]@22:01 - Rejected the first helper draft after adversarial review found a Region out-of-bounds regression and false-positive tests.
	- **Evidence:** Region Slicer permits up to 256 slices while `manualSliceDrums` has 64 entries. Forming `manualSliceDrums[i]` as a helper argument occurs before the helper sees `isManualSlice == false`, so Region slice 65 and above reaches undefined behavior. The helper tests call only the helper; all still pass if the production call is removed, returned to the existing-Kit branch, or never reaches confirmation. Production never uses the helper's failed-placement enum value.
	- **Effect:** Remove the header, enum, helper calls, unit file, and CMake registration. Restore direct first-drum publication and place one direct `if (isManualSlice)` assignment after the later-drum placement branches. This preserves failure cleanup and makes Region avoid the array access entirely.
	- **Next:** Repair the temporary draft, run focused formatting/compile checks, and return the smaller two-line behavioral diff for final adversarial re-review.
- [x] [[2026-08-24]]@22:04 - Folded the final one-file P34 correction after clean re-review.
	- **Evidence:** Temporary commit `e1b985c5a610` reduced to one `slicer.cpp` hunk and passed formatter, UnitTests rebuild, affected ARM Slicer object compilation, and final adversarial review. Jujutsu folded it into `nl/patch-sample-slicer-04`, change `pnvzntvmqkqktuxryzpuoyowvxkvmrpv`, commit `2ca86e6185d835cd933474518f8d2f13967b8b67`, and rebased 11 descendants without conflict.
	- **Effect:** Every later Manual Slice drum is retained after either placement branch succeeds; existing-Kit failure returns before publication; Region does not evaluate the Manual array. The rejected helper and proxy tests leave no residual source. Full host and Release gates wait only to combine with P23's immediately adjacent Slicer change.
	- **Next:** Implement P23 from the clean empty tip, then validate and rebuild once before physical Manual and Region Slicer checks.

- [x] [[2026-08-24]]@22:20 - Completed the shared P23/P34 validation and Release build.
	- **Evidence:** Post-fold review confirmed the final P34 write remains inside `if (isManualSlice)` before `manualSliceDrums[i]` access, existing-Kit failure returns before that point, and Region never accesses the 64-entry Manual array. The final bookmark is `52485167b3363c2e75b43ac01156eaba5f9fa6a1`; the focused P23 suite passed; configured CTest passed 11 of 11; formatting passed; ARM build inputs were current; and the single logged Release build passed in 89 seconds.
	- **Effect:** The local image contains the deterministic Manual Slice crash repair and the reviewed Select menu. Software gates are complete; the full Slicer lifecycle still requires the physical Deluge.
	- **Next:** Install only if the operator chooses, then re-test new-Kit Manual 2+ slices, existing-Kit Manual, and Region above 64.

## Validation

- **Command or action:** New-Kit Manual Slice confirm regression.
	- **Expected result:** Every generated slice receives its requested start, end, and transpose without a null access; existing-Kit placement remains unchanged.
	- **Observed result:** Independent source review and post-fold integrity review passed. Configured CTest passed 11 of 11, the linked Release map contains the final Slicer sources, and the Release image completed with `deluge.bin` SHA-256 `2e4d8d9f4728e2dd0d1587b8c50808261417a60631201ed5a050e5d60c6050c0`.
	- **Failure signals:** A null generated-drum entry, changed Region Slice placement, duplicate row assignment, or cleanup regression after allocation failure.
	- **Interpretation:** Passing proves the identified cross-function crash is repaired; physical Slicer behavior remains a later device check.

## Unfinished Work

- [x] Implement, fold, and validate the focused Slicer correction.
- [ ] Confirm the repaired lifecycle on the physical Deluge.
