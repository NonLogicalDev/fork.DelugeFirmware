---
description: "Active plan to add explicit per-batch playback-mode controls to Slicer."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Choose slice playback mode in Slicer

## Purpose

Let a player choose how a newly confirmed Slicer batch plays without leaving the Slicer or changing the device-wide default.

## Product integration

- Existing model: generated slice playback follows the device default for longer slices, while the established short-slice rule forces Once. Slicer already exposes the pending batch choice through the top three pads of the far-right status column as Default, All Cut, and All Once.
- User intent: make the same decision available from Select through a compact `AUTO`, `CUT`, `ONCE` menu in both normal/Region Slicer and Manual/Lazy Slicer.
- Integrated model: one pending batch playback choice owns two synchronized input surfaces. Select opens a single-choice `AUTO`, `CUT`, `ONCE` confirmation menu in either Slicer mode; the existing right-column pads remain direct shortcuts to the same state. Turning Select changes the highlighted item only. Pressing Select on an item updates the shared pending choice and immediately runs the existing slice-confirmation path. `AUTO` is the existing Default policy, while `CUT` and `ONCE` remain whole-batch overrides.
- Existing behavior preserved: Auto retains the present playback-mode rule, including the short-slice Once rule. Cut and Once are explicit per-batch overrides; they do not change the stored device default or existing Kit rows. Back cancels the menu and restores the unchanged Slicer session, including all boundaries, source, mode, preview state, and pending choice. Merely opening or navigating the menu does not confirm or mutate the batch.

## Scope

- In scope: three far-right vertically arranged controls plus a Select-opened `AUTO`, `CUT`, `ONCE` menu in both normal/Region and Manual/Lazy Slicer modes; synchronized selected-state feedback; applying the chosen mode consistently to every confirmed row in one batch.
- Keep stable: slice detection, slice boundaries, preview behavior, names, Kit placement, source-file ownership, device-wide defaults, and existing rows.
- Out of scope: changing the configured Default Slice Mode, adding a global mode preference, changing non-Slicer sample behavior, firmware installation, flashing, publication, or upstream submission.

## Completion Criteria

- [x] Slicer shows Default, All Cut, and All Once as vertically arranged far-right controls in both Region and Manual modes without hiding an existing editing control.
- [x] Default preserves the established generated-slice playback rule.
- [x] All Cut makes every confirmed slice in the new batch use Cut, including short slices.
- [x] All Once makes every confirmed slice in the new batch use Once.
- [x] Selecting a control does not mutate device defaults, existing Kit rows, or an unconfirmed batch.
- [x] `DEVIATIONS.md` states the final observable contract without code.
- [x] Focused source review, applicable host checks, and a local Nix Release build pass. No flashing or publication occurs.
- [x] Select opens the same `AUTO`, `CUT`, `ONCE` batch-mode confirmation menu from both normal/Region and Manual/Lazy Slicer without immediately confirming, exiting, or changing slice boundaries.
- [x] The menu initially reflects the current pending batch choice; pressing Select on an item updates the same state as the right-column controls and runs the existing confirmation path, while Back returns to the unchanged Slicer session.
- [x] Auto, Cut, and Once preserve the established whole-batch behavior and remain synchronized with the existing direct pad shortcuts on both supported display types.
- [x] The updated contract, focused checks, adversarial review, configured host suite, affected ARM object, and new Release build pass after folding into `nl/patch-sample-slicer-04`.

## Blockers

None. The Slicer leaves the right-hand status column to the underlying view. Back already exits Slicer, so its top three status pads can safely become the batch-mode controls while the audition column remains untouched.

## Current State

- Status: software-complete in `nl/patch-sample-slicer-04` at `52485167b3363c2e75b43ac01156eaba5f9fa6a1`. Both Slicer modes share the Select menu and right-pad batch state; held-control releases remain owned by Slicer; local review, host tests, formatter, ARM build, and Release build pass.
- Next action: physically test both Slicer modes, both display types, Back/accept/failure, and held Manual audition-pad and Horizontal encoder releases. The agent will not flash the device.

## Workers

- `/root`: completed P23 product integration, sequencing with P34, Jujutsu folding, final combined validation, and user communication; physical-device validation remains.
- `/root/slicer_select_menu_audit`: completed the read-only Select/menu audit; confirmed Select is the only confirmation control, identified destructive `focusRegained()` re-entry, reusable `ContextMenu` patterns, existing localization, and the exact P34 collision.
- `/root/slicer_select_menu_impl`: completed the menu, lifecycle split, shared resolver/tests, and release-ownership repair in temporary change `stqvqsksppry` before folding.
- `/root/slicer_select_menu_review`: rejected the first draft for swallowed Manual pad and Horizontal encoder releases, passed the repair, and passed the post-fold integrity check with no remaining software finding.

## Artifacts

- **A1:** Slicer playback-mode controls.

	- **Type:** Firmware source and local Jujutsu revision.
	- **State:** Implemented and locally validated.
	- **Contents:** Default, All Cut, and All Once controls with batch-scoped confirmation behavior.
	- **Location:** Slicer UI and generated-slice confirmation sources.
	- **SCM surface:** Folded into `nl/patch-sample-slicer-04`, change `pnvzntvmqkqktuxryzpuoyowvxkvmrpv`, commit `52485167b3363c2e75b43ac01156eaba5f9fa6a1`.

- **A2:** Self-contained deviation contract.

	- **Type:** Behavior specification.
	- **State:** Implemented.
	- **Contents:** Control placement, selected behavior, overrides, and preservation boundaries.
	- **Location:** `DEVIATIONS.md` in the newest local contract revision.
	- **SCM surface:** Maintained at the stack tip in `nl/patch-oled-02`, commit `1d7c88b1d2c276c87c2288c23a826d4aa71a667c`.

## Decisions

- **Decision:** Treat the buttons as a per-batch decision, not a change to the global default.

	- **Rationale:** The player needs an immediate local choice without silently changing later Slicer sessions or other sampling actions.

- **Decision:** Preserve the current short-slice Once behavior for Default.

	- **Rationale:** Explicit All Cut is the appropriate way to override a deliberate short-fragment safety rule.

- **Decision:** Treat `AUTO` as the menu label for the existing Default policy.

	- **Rationale:** The user requested `AUTO`, and it describes the current rule more clearly: use the configured/default behavior, including the established short-slice Once safeguard.

- **Decision:** Keep the right-column pads and Select menu synchronized rather than creating a second playback-mode setting.

	- **Rationale:** Both controls act on the same unconfirmed batch. Separate state would let the menu and pads disagree and make confirmation order-dependent.

- **Decision:** Make the Select menu the confirmation step rather than a detached settings overlay.

	- **Rationale:** Select is already the only Slicer confirmation action. Opening the chooser on the first press and confirming the highlighted mode on the second preserves that role. A chooser that only sets state would make the next Select reopen itself and leave no way to finish.

- **Decision:** Return from menu cancellation without re-running Slicer entry initialization.

	- **Rationale:** Generic ContextMenu dismissal calls the parent UI's `focusRegained()`, but Slicer currently uses that callback to clear slice arrays, reload mode/source state, and rebuild boundaries. Back must restore the exact in-progress session, not silently reset it.

- **Decision:** Split Slicer's one-time session initialization into `opened()` and leave `focusRegained()` as non-destructive display restoration.

	- **Rationale:** This follows the existing Sample Browser lifecycle model and makes ordinary menu cancellation safe without adding a one-off bypass flag. Entry still clears undo, establishes mode/source state, resets boundaries and pending mode once; returning from an overlay only redraws the preserved session.

- **Decision:** Close the mode menu before invoking Slicer's extracted confirmation operation.

	- **Rationale:** Successful slicing closes the entire Sample Browser/Slicer stack. Closing the mode menu afterward could act on a UI object that is no longer open. On confirmation failure, the already-closed menu returns to the preserved Slicer and the existing error remains visible.

- **Decision:** Reuse existing Auto, Cut, and Once localization and standard ContextMenu rendering.

	- **Rationale:** The repository already supplies the three labels and ContextMenu handles OLED highlighting, option clamping, 7SEG wrapping, Back cancellation, and Select acceptance. New strings or custom rendering add no behavior the user requested.

- **Decision:** The menu may consume new pad and control presses, but it must forward releases for controls Slicer already owns.

	- **Rationale:** Opening an overlay does not cancel a held Manual audition pad or Horizontal encoder press. Swallowing their later releases can leave audio sounding or coarse-scroll state latched. Forwarding only the release preserves ownership without letting new presses edit Slicer behind the menu.

## Implementation Steps

- [x] Trace Slicer's rightmost pad ownership, drawing, input routing, and existing feedback patterns.
- [x] Add the three controls and selected-state feedback without taking an existing editing pad.
- [x] Apply the chosen batch mode at confirmation for the anchor and every added slice.
- [x] Update `DEVIATIONS.md`, fold the source into `nl/patch-sample-slicer-04`, and review the complete diff.
- [x] Run focused host checks and a local Nix Release build.
- [x] Trace Select ownership and reusable three-choice menu patterns in both Slicer modes and both display types.
- [x] Split one-time Slicer entry initialization from non-destructive focus restoration, then open the menu from Select and bind it to the existing pending batch choice.
- [x] Close the menu before applying the selected mode and invoking the extracted existing confirmation operation; preserve cancellation and failed-confirmation state.
- [x] Add focused state/menu tests, adversarially review the combined P23/P34 Slicer diff, then fold and repeat host, ARM, formatter, and Release gates once.

## Work Log

- [x] [[2026-08-22]] - Started after the user requested far-right Slicer controls arranged vertically as Default, All Cut, and All Once.

	- **Effect:** The playback-mode choice is explicit and local to a new slice batch; it does not replace the parallel investigation of an unexpected Once result for a long slice.
	- **Next:** Inspect current Slicer grid ownership before selecting implementation details.

- [x] [[2026-08-22]] - Verified the physical control surface available to Slicer.

	- **Evidence:** `Slicer` owns all 16 main-grid columns but does not own the two right-hand sidebar columns. The first sidebar column currently reaches the underlying status/mute action; the second remains the audition column.
	- **Effect:** Use the first sidebar column's top three pads, in the requested top-to-bottom order. Preserve main-grid editing and the audition column.
	- **Next:** Add selected-state rendering, input routing, and one batch-mode resolver used by all generated rows.

- [x] [[2026-08-22]] - Implemented the three-mode batch selector and folded it into the existing Slicer revision.

	- **Evidence:** The top three status pads select Default, All Cut, or All Once; their colors show the selected choice and each press gives OLED or seven-segment feedback. Both the anchor and every added drum use the same batch-mode resolver at confirmation. The Manual preview's temporary Once mode now restores when the player exits or leaves Manual mode.
	- **Effect:** An explicit All Cut selection is available even for short slices, while Default retains the prior short-slice rule. No stored default or existing row is changed by merely selecting a button.
	- **Next:** Test all three choices on the physical Deluge, including a cancelled Manual preview and a second existing-Kit batch.

- [x] [[2026-08-24]]@21:54 - Reopened P23 for a Select menu in both Slicer modes.

	- **Evidence:** The operator requested `AUTO`, `CUT`, and `ONCE` on Select and clarified that it must work in both normal/Region Slicer and Manual/Lazy Slicer.
	- **Effect:** The menu becomes a second interface to the established per-batch choice, not a new preference. Existing right-column controls and confirmation behavior remain unless source review finds a direct Select conflict that requires explicit product resolution.
	- **Next:** Complete a read-only menu/dispatch audit while P34 owns Slicer source edits, then integrate after the crash repair passes review.

- [x] [[2026-08-24]]@21:57 - Preserved Select confirmation and in-progress Slicer state in the menu model.

	- **Evidence:** Slicer's current `SELECT_ENC` press is the only successful confirmation route. Generic menu closure invokes parent `focusRegained()`, while Slicer used that callback as destructive entry initialization.
	- **Effect:** First Select opens the chooser; Select on the highlighted item applies the choice and confirms; Back cancels and restores the unchanged session without reinitializing Slicer. The implementation must separate menu return from entry initialization or use a return path that explicitly preserves state.
	- **Next:** Identify the narrowest existing menu class and return hook that supports this lifecycle, then add focused menu-state and cancellation regressions.

- [x] [[2026-08-24]]@22:02 - Accepted the dedicated ContextMenu and Slicer lifecycle split.

	- **Evidence:** Existing ContextMenu supplies both display implementations, Back cancellation, Select acceptance, and three-option navigation. Existing Auto, Cut, and Once strings need no localization change. Sample Browser demonstrates the clean `opened()` versus `focusRegained()` lifecycle. Successful confirmation closes the Slicer stack, so the menu must close before calling the extracted confirmation body. P34 currently overlaps `slicer.cpp` and `tests/unit/CMakeLists.txt`.
	- **Effect:** P23's minimal edit set is Slicer lifecycle/confirmation access, one dedicated fixed menu, one allocation-free shared mode resolver with focused tests, CMake registration, and the existing Deviation 5 contract. Implementation waits until P34 folds to avoid overlapping edits and misleading validation.
	- **Next:** Complete and fold P34, then create a fresh temporary child for P23 and validate opening, initial selection, cancellation, success, failure return, both modes, both displays, and right-pad synchronization.

- [x] [[2026-08-24]]@22:18 - Rejected the first P23 draft after adversarial release-ownership review.

	- **Evidence:** Manual preview does not enter a separate audition UI mode, so Select can open the menu while its pad is held; generic ContextMenu consumes the later pad release instead of stopping preview. The same overlay can open while the Horizontal encoder is held, and generic ContextMenu does not forward its release, leaving Slicer's pressed/coarse state stale.
	- **Effect:** Do not fold or rebuild this draft. Preserve the menu's ownership of new input while forwarding only already-owned Slicer releases, then re-review the complete UI transition.
	- **Next:** Repair pad and Horizontal encoder release routing in the dedicated Slicer menu, run focused compilation/formatting, and send the exact diff back to adversarial review.

- [x] [[2026-08-24]]@22:20 - Repaired release ownership, folded P23, and completed the combined P23/P34 gate.

	- **Evidence:** The dedicated menu forwards pad releases and only Horizontal encoder releases to Slicer while keeping presses inside ContextMenu. Re-review and post-fold integrity review passed. Jujutsu folded the source and test into `nl/patch-sample-slicer-04` at `52485167b3363c2e75b43ac01156eaba5f9fa6a1`, kept the self-contained contract at tip `1d7c88b1d2c276c87c2288c23a826d4aa71a667c`, and left an empty clean working change with no conflicts. The focused suite passed 6 tests and 20 checks; configured CTest passed 11 of 11; formatting passed; the ARM objects and linked image include the new menu; the single logged Release build passed in 89 seconds.
	- **Effect:** Both normal/Region and Manual/Lazy Slicer now use the same Auto, Cut, Once confirmation menu without resetting the session or latching a held control. The first reviewed draft leaves no residual patch.
	- **Next:** Physically test menu legibility, playback modes, cancellation, failed confirmation, and held-control releases before choosing whether to install the local image.

## Validation

- **Command or action:** Focused source review, host checks, local Nix Release build, and later physical Deluge check.

	- **Expected result:** Each control is visibly selectable, applies its intended mode to an entire newly confirmed batch, and leaves the device default and existing rows unchanged.
	- **Observed result:** The final focused resolver suite passed 6 tests and 20 checks; configured CTest passed 11 of 11; clang-format passed; post-fold integrity and adversarial review passed; and the linked map contains the new Slicer menu object and symbols. The single logged Nix Release build completed in 89 seconds with text 1,643,516 bytes, data 67,828 bytes, and BSS 364,344 bytes. `deluge.bin` SHA-256 is `2e4d8d9f4728e2dd0d1587b8c50808261417a60631201ed5a050e5d60c6050c0`; `deluge.elf` SHA-256 is `35ef65a727f21fa962ebb57862eb9ff7872bc1b94fd02ed8b26f4ec90332aa40`.
	- **Failure signals:** A control obscures slice editing, a mode leaks into later batches, short-slice Default behavior changes, an existing row changes, or a build/test regression occurs.
	- **Interpretation:** Local checks prove integration; physical confirmation is still required for pad layout and playback behavior.

## Unfinished Work

- [x] Implement and locally validate the three Slicer playback-mode controls.
- [x] Add and locally validate the Select-opened `AUTO`, `CUT`, `ONCE` menu in both Slicer modes.
- [ ] Confirm the behavior on a physical Deluge. Do not flash automatically.
