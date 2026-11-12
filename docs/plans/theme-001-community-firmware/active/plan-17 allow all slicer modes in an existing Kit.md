---
description: "Active plan to let all Kit-creation sample actions start at a selected top Sound Drum."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Allow all slicer modes at a Kit's top

## Purpose

Let a player add samples to an existing Kit with Load all, Slice, or Manual slice beginning at the selected topmost Sound Drum.

## Product integration

- Existing model: an empty-pad browser entry creates a temporary Sound Drum immediately, then treats that new row as the sample-creation anchor. The current availability guard rejects an already-populated selected Sound Drum even when every row above it is available.
- User intent: one clear reuse rule, rather than a surprising difference between the three choices. The selected Sound Drum is the first destination for the new material, and its next row upward begins the empty capacity check.
- Integrated model: if every Kit row above the selected Sound Drum is empty, the selected action replaces that Sound Drum with the first generated sample and appends later rows above it. The pre-existing empty-pad route uses the same rule.
- Architecture impact: generalize the availability predicate to any selected Sound Drum and preserve the established slice and folder-generation paths. Do not create a second anchor or change ordinary browser loading.
- Existing behavior preserved: a brand-new Kit still offers its existing choices. A selected Sound Drum with any assigned row above it remains unavailable.

## Scope

- In scope: make Load all, Slice, and Manual slice available when a selected Sound Drum has no assigned Kit row above it; use that row for the first added sample; append every additional generated row above it. Retain the established selected-empty-pad route.
- Keep stable: existing sample formats, selection rules, Slice analysis, Manual Slice editing, source-file loading, ordinary empty-Kit creation, playback, row contents, saving, and MIDI/Gate behavior.
- Out of scope: inserting into the middle of a Kit, overwriting any row other than the selected Sound Drum, moving existing rows, a new slicing algorithm, changing generated slice names, Manual Slicer undo/redo, firmware flashing, installation, release publication, or upstream submission.

## Completion Criteria

- [x] A selected topmost Sound Drum exposes every applicable existing sample-creation option, not only Manual slice, while the established selected-empty-pad route remains available.
- [x] Load all, Slice, and Manual slice each preserve every row below the selected Sound Drum, including position, sound, notes, and settings.
- [x] The selected Sound Drum receives the first added sample and subsequent rows are placed above it for every supported entry path.
- [x] Invalid anchors retain clear rejection and do not alter the Kit.
- [x] `DEVIATIONS.md` describes the unified reuse rule without source-level instructions.
- [x] Focused review, host checks, and a local Nix Release build pass after the selected-anchor correction. No flashing or publication occurs.

## Blockers

No source blocker remains. A physical Deluge must confirm that a selected Sound Drum at pad 7 accepts each action when pads 8 and above are empty, then refuses the same action when any row above is assigned.

## Artifacts

- **A1:** Unified existing-Kit creation source change.

	- **Type:** Firmware source and local Jujutsu revision.
	- **State:** Implemented and locally validated; physical confirmation pending.
	- **Contents:** The selected Sound Drum may already have a sample. It receives the first generated sample when every row above it is empty.
	- **Location:** Existing sample-browser and Slicer entry sources.
	- **SCM surface:** Folded into `nl/patch-sample-slicer-04` at `c2ee49beb216`.

- **A2:** Self-contained deviation contract.

	- **Type:** Behavior specification.
	- **State:** Implemented.
	- **Contents:** Availability, placement, preservation, and invalid-anchor boundaries.
	- **Location:** `DEVIATIONS.md` in the newest local revision.
	- **SCM surface:** `DEVIATIONS.md` is in the newer `nl/patch-undo-01` contract revision at `6c971ec46407`.

## Decisions

- **Decision:** Treat the topmost empty pad as the single reuse rule for every existing Kit-creation choice.

	- **Rationale:** The pad unambiguously marks where new material may be appended without hiding different safety rules behind menu labels.

- **Decision:** Fold source changes into `nl/patch-sample-slicer-04`.

	- **Rationale:** This revises the established existing-Kit entry behavior in the same sample-slicer component. The local stack should contain the final behavior rather than a follow-up correction patch.

## Implementation Steps

- [x] Trace availability and placement for Load all, Slice, and Manual slice in both new and existing Kits.
- [x] Generalize the valid-anchor rule and append behavior without changing empty-Kit behavior.
- [x] Correct the selected-pad / Pad + 1 accounting regression without changing the accepted top-empty placement rule. In particular, selecting the first empty pad above an existing slice set must admit another slice batch.
- [x] Update `DEVIATIONS.md` and local standing guidance, then fold the source change into `nl/patch-sample-slicer-04`.
- [x] Review the final diff, run host checks and a local Nix Release build, and record exact results.
- [x] Generalize the current selected-top predicate to any selected Sound Drum, without changing generation or browser-loading paths.
- [x] Rebuild and run the host checks after the selected-Sound-Drum change.
- [ ] Physically re-test Manual Slice on both a new Kit and a selected Sound Drum with an empty run above it.

## Work Log

- [x] [[2026-08-22]] - Started after the user clarified that the top-empty-pad reuse rule must apply to every existing sample-creation choice.

	- **Effect:** The prior Manual-only rule is superseded as the final product behavior.
	- **Next:** Trace the standard Slice and Load all paths before changing their availability or placement.

- [x] [[2026-08-22]] - Generalized the valid selected-top-empty-pad rule to every Kit-creation choice.
	- **Evidence:** `nl/patch-sample-slicer-04` at `8e2e2b2d32aa`; source review confirmed direct anchor-and-above placement for Load all and both Slice modes.
	- **Effect:** The menu and shortcut now share one safe reuse rule. An existing-Kit Slice session remains in its selected mode so its sample source remains valid.
	- **Next:** Confirm menu availability, direct shortcut behavior, and row placement on a physical Deluge.

- [x] [[2026-08-22]] - Physical testing reported broken Pad + 1 accounting after the recent anchor-placement change.
	- **Effect:** Local source and host checks are insufficient for the selected-pad mapping. The placement criteria are reopened until the exact off-by-one behavior is fixed and rebuilt.
	- **Next:** Trace the selected Kit pad through eligibility and row insertion, apply the smallest correction, then re-run host and Release checks.

- [x] [[2026-08-22]] - Physical testing narrowed the failure to adding a second slice set at the empty pad above the first.
	- **Evidence:** The browser's only pre-entry rejection is `canUseKitSampleCreationOnTopEmptyPad()`. Its current condition uses the selected drum's audio-file holder as an emptiness proxy, even though the intended invariant is the selected top empty row.
	- **Effect:** Do not change the anchor-and-above placement rule. Replace the unreliable eligibility proxy only after tracing the creator and browser-preview state.
	- **Next:** Prove the source of the holder state, make the smallest safe eligibility correction, then rebuild the exact image for device retest.

- [x] [[2026-08-22]] - Replaced the temporary holder-state test with a browser-session anchor identity check.

	- **Evidence:** `nl/patch-sample-slicer-04` at `5aa6c50c9218`; only the row created from the selected empty Kit pad can use the existing-Kit creation route during that browser session.
	- **Effect:** Previewing or loading state no longer prevents the empty pad directly above a slice set from starting the next sample-creation action. A pre-existing sound row cannot gain this permission.
	- **Next:** On the device, create a first slice batch, select the empty pad immediately above it, and verify that Slice and Manual slice enter while an occupied pad remains rejected.

- [x] [[2026-08-22]] - Physical testing confirmed second-set entry and no confirmation freeze, but found that the second Slicer session inherits a tail-range waveform from the first slice set.

	- **Effect:** The availability correction is valid, but the selected file is not yet the sole source of the second session's visible and audible range.
	- **Next:** Trace range start/end initialization and waveform navigator handoff for a freshly created reuse anchor; preserve the selected file while clearing only stale per-slice bounds.

- [x] [[2026-08-22]] - Bound every existing-Kit Slicer session to its selected file before opening the Slicer.

	- **Evidence:** The shared browser-to-Slicer entry now loads the selected file onto the fresh anchor for Manual Slice and for any existing-Kit Slice, then resets the waveform navigator from that anchor rather than a global preview.
	- **Effect:** A second batch cannot inherit the previous batch's browser-preview waveform or range. The direct empty-pad creation path also now puts the actual NoteRow identifier, not its transient array position, into its model stack.
	- **Next:** Build and run the host checks, then repeat the physical two-batch test with a clearly different second source file.

- [x] [[2026-08-22]] - Rejected the speculative browser-anchor repair before folding it into a revision.

	- **Evidence:** The physical workflow selects an existing Sound Drum at pad 7, checks that pads 8 and above are empty, and expects slice one to replace pad 7. The source only authorizes a temporary anchor that was created by choosing an empty pad before entering the browser.
	- **Effect:** The selected existing Sound Drum is the intended first destination, not a protected base row. The uncommitted guard change and its local build were discarded; the revision stack is unchanged.
	- **Next:** Generalize the predicate to the selected Sound Drum and keep the existing generation path, which already writes slice one to it and later slices above it.

- [x] [[2026-08-22]] - Replaced the temporary-empty-anchor restriction with the selected-Sound-Drum rule.

	- **Evidence:** `canUseKitSampleCreationOnTopEmptyPad()` now finds the selected drum's row and rejects only when a Drum occurs at a higher row. `Slicer::doSlice()` and `importFolderAsKit()` already use that selected drum for item one and insert later rows at increasing row indexes. The obsolete browser-session anchor state was removed.
	- **Validation:** `nl/patch-sample-slicer-04` at `c2ee49beb216`; local Nix Release ELF/BIN regenerated at 11:56 local time; configured CTest suite passed 7/7.
	- **Effect:** With pad 7 selected, a pre-existing sample at pad 7 no longer blocks the action. Slices load into pads 7, 8, 9, and upward only if pads 8 and higher contain no Drum.
	- **Next:** Confirm that exact populated-pad-7 case on the physical device, then test rejection when pad 8 or another higher row is assigned.

## Validation

- **Command or action:** Focused source review, configured host CTest suite, local Nix Release build, and later physical Deluge check.

	- **Expected result:** Every applicable option appears for a selected Sound Drum with no assigned row above it, slice one replaces that selected row, generated rows then grow upward, and existing rows below remain unchanged.
	- **Observed result:** The selected-Sound-Drum correction is folded into `nl/patch-sample-slicer-04` at `c2ee49beb216`; the configured CTest suite passed 7/7 and the local Nix Release build regenerated the ELF and BIN at 11:56 local time. The temporary-anchor state was deleted after source review. Physical confirmation remains pending.
	- **Failure signals:** A valid option is missing, an action modifies a row below the selected Sound Drum, generated rows fail to start at the selected Sound Drum, an occupied row above is allowed, or a build/test regression.
	- **Interpretation:** Local checks establish source integration; a physical device is still needed to confirm menus and row placement.

## Unfinished Work

- [ ] On a physical Deluge, test each option from a selected Sound Drum with an empty run above it, the selected-empty-pad route, and rejected occupied-above cases. Do not flash automatically.
