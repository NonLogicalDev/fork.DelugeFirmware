---
description: "Open a shared Song Root / Mode menu with Shift + Scale while preserving tool-specific controls."
tags:
  - note-type/project
  - deluge
  - firmware
---

# Open Root and Mode menu with Shift Scale

## Purpose and authority

Make the Song key easy to inspect and edit through a Root / Mode menu. The operator approved local implementation and owner migration on [[2026-09-12]]. Owner authority is in [OWNERS.md](../../OWNERS.md). This menu replaces the earlier encoder-control proposal in [[plan-45 navigate scale roots and types with Scale encoders]]; do not implement the two new encoder shortcuts.

## Current State

- Status: corrected implementation software-complete; hardware confirmation pending. Source, catalog, independent review, host/ARM checks and final Release build are finished. Native pad/Vertical delegation replaces the discarded audition-only design.
- Finish point: local revision, focused checks, independent review and one uninterrupted Release build; physical verification remains human-controlled.
- Fresh feature change: `zynqpn`, created above catalog revision `3d2dba95155e`; intended bookmark `nl/patch-scale-controls-01`.

## Product integration

- The Song already owns root and mode; views currently expose inconsistent Scale shortcuts, and Velocity Drums deliberately uses Scale for velocity controls.
- Shift + Scale should enter one two-item Root / Mode menu, not introduce per-view key state or another independent scale model.
- Prefer existing menu controls and OLED/7SEG conventions. Root edits use established root-selection semantics (which may infer a new mode), not implicit song transposition; always refresh both resulting values. Mode selection uses existing Song scale mapping and validity constraints.
- Preserve musical-view scope and explicit tool exceptions, and own the initiating Scale release so opening/closing the menu never also cycles or toggles scale.
- Use a small dedicated menu UI with familiar Select/Back controls. The existing Sound Editor setup depends on clip-specific state; routing Song-level views through it would require unrelated setup special cases. Reuse Song operations, not its clip-dependent editor wrapper.

## Accepted behavior

- Final hardware-feedback direction: keep Root and Mode tightly spaced near the top of the OLED. Pass every pad event and Vertical turn directly to the underlying native view with its existing modifiers, note handling, editing and navigation. Do not invent an audition-only mode, pad filtering, note tracking or synthetic releases. Keep Select/Back and the menu display local; repaint only if the menu remains current after native input. Preserve native action undo using the underlying root as the action context. Decline key edits while pads/native editing gestures are active so their eventual releases retain the original mapping. Fold into the existing feature, update catalog/contracts, review and rebuild.

- Shift + Scale from idle melodic Instrument Clip, Keyboard and Clip Automation opens the menu instead of cycling scales. Session Rows/Grid, Arranger, Arranger Automation and Performance expose the same Song menu in idle state. Do not use an arbitrary selected Clip as the Song target.
- Root offers all 12 pitch classes; Mode offers enabled valid preset scales and saved User scale when available. Represent an existing custom scale truthfully. Rejected scale changes preserve prior state.
- Turn Select to choose Root or Mode; press Select to edit it; turn Select to change its value; Back returns to the menu, then the originating view. Use ordinary display conventions on OLED and 7SEG.
- Opening, viewing and closing do not change key, toggle scale mode, transpose notes, clear undo or affect playback. Actual edits retain established root/scale semantics; do not promise new undo support. No-op and rejected edits do not clear history.
- Preserve Shift + Scale Sound Editor horizontal-menu navigation, Velocity Drums Scale profile/fixed-velocity controls, other Kit Scale behavior, Learn/pad/pressed-encoder/Load combinations, current menus, browsers, sample editors, recorder and Slicer. Do not add the proposed Scale + encoder aliases in this change.
- No new persistence, save schema, timer or audio-path processing. No flashing, installation, publication or upstream submission.

## Completion Criteria

- [ ] Entry/view/return behavior works on the declared surfaces, with both Shift/Scale release orders and early menu closure accounted for.
- [ ] Correct Root/Mode values, root inference, enabled/User/custom scales, invalid/no-op edits and rendering behavior have focused evidence.
- [ ] Existing excluded controls and playback are preserved.
- [x] Source formatting, focused tests, host suite, independent review and one local Release build pass.
- [x] Catalog, self-contained deviation contract and recovery records describe the final behavior.
- [ ] Physical OLED and 7SEG checks completed by the operator (not required to claim software-complete).

## Work Log

- Corrected final Release passed in 53 seconds, ending `2026-09-13T02:03:25Z` (2026-09-12 local). Binary `build/Release/deluge.bin`: 1,743,260 bytes, SHA-256 `3fa56c8ab50f0f06162949b69deb1c3fcbaa6b2492c051df00f8477f0f32ae84`. Checkpoint `87172eb75934f9804b0eb755b00232a4f0ae89d0`, same `nl/patch-scale-controls-01` bookmark. No flashing. Owner doctor: 5 groups, 52 plans, zero errors/warnings. Remaining task is physical confirmation of compact readability and native-mode input.

- Corrected implementation verified: seven original policy tests and all 23 host suites passed; three affected ARM objects compiled. Independent review passed after guarding key edits during active native gestures and preserving root action undo for every delegated action type. The final follow-up contains no pad gate, note-input seam, synthetic note-offs or Automation/Instrument Clip lifecycle changes. Root/Mode rows are at y16/26. Catalog: 1,273 bindings, 391 definitions, 518 coverage rows across 184 files; zero gaps/drift and 12 regression tests pass. Folded checkpoint `87172eb75934f9804b0eb755b00232a4f0ae89d0` remains `nl/patch-scale-controls-01`. Final build log `/private/tmp/deluge-scale-menu-native-release.log`; no source edits during build.

- Final correction supersedes all earlier follow-up designs below: the operator explicitly rejected custom pad routing. Removed the pad gate, note-input seam, synthetic note-off and Automation/Instrument Clip lifecycle changes. Native pad and Vertical delegation is the final design. Review found two narrow integration issues to fix: native actions must retain undo under the menu, and key edits must not remap held native gestures. Earlier proposed restrictions and recording refactors below are historical, not current instructions.

- [[2026-09-12]] hardware follow-up: compact rows at OLED y16/26, keep playable pads visible, allow only ordinary keyboard main pads or Clip/Clip Automation audition column plus plain Vertical rotation. Root/Mode changes end menu-owned held notes through their native release path before remapping, requiring lift/repress afterward; normal menu closure retains native note state for release in the root view. Use narrow input seams to bypass shortcut dispatch without duplicating note playback/recording code. Modified presses stay blocked and later releases still clean up. Update affected source catalog records after source freezes, review and rebuild once.
- Follow-up review identified two required integration checks: the Chord layout can replace unsupported scales on refresh, and ActionLogger normally rejects record actions under an overlay. Preserve the edited Song key using a compatible keyboard layout if the current one cannot represent it, with explicit feedback. Preserve ordinary recording undo by allowing only the required record action with the actual root view as its return target, not the menu. Do not add global pad-release interception: source review confirms native releases are safe after the menu has already stopped those notes.
- Release review qualification: Instrument Clip and Keyboard already guard duplicate releases, but Automation records a note-off before its pressed-state check. Give that native audition path the smallest inactive-release guard; do not send generic Automation grid-editor input from this menu. Verify ordinary Automation audition remains intact.
- Final recording decision: reuse Instrument Clip's existing per-row recording lifecycle for Automation audition, not a private duplicate tracker. Native `FOLLOW_UI` would lose a recorded release if Horizontal is pressed later; the shared owner preserves that matching edge and transfers held notes across Clip/Automation and native scrolling. Independent review must verify this seam before build.

- [[2026-09-12]]: verified legacy ownership and explicit migration approval, created this active plan after OWNERS.md was established. The previous audit found Velocity Drums and Sound Editor collisions plus Scale-release and undo side effects; preserve those exceptions and avoid opening-menu mutations.
- [[2026-09-12]]: source first pass delivered; host configure/build/test launched with a durable log at `/private/tmp/deluge-scale-menu-host-tests.log`. Independent source review runs before Release. Next integration step is to update affected JSONL bindings, source coverage and snapshots, without expanding the catalog schema or changing unrelated bindings.
- [[2026-09-12]]: host configure/build and all 23 CTest suites passed, including seven new gesture/candidate unit tests. These test pure policies, not full UI/Song integration. Independent reviewer found a missing connected-controller root notification; fixed and re-reviewed with no remaining findings. Final review also covers eligibility guards, release ownership, underlying display repaint and current menu type usage. ARM object check caught a `std::clamp` integer-type mismatch; explicit `int32_t` fixed it. Recheck passed (`/private/tmp/deluge-scale-menu-arm-recheck.log`). Source is frozen before the single Release build. Physical navigation, display readability, complete Song undo behavior and live audio remain unverified.
- [[2026-09-12]]: catalog integration verified by main: 1,270 bindings, 389 scanner definitions, 514 coverage rows across 183 files; zero unaccounted candidates, source drift or invalid coverage; all 12 regression tests passed. Nine menu/lifecycle bindings added and two existing Scale-cycle predicates updated. Checkpoint `cd2701d953ba59cdd1ba4ea447f88c58fa5d44f9`, bookmark `nl/patch-scale-controls-01`; AGENTS.local.md remains ignored and is also reflected in owner guidance. One final Release build started at `2026-09-12T23:34:03Z`; durable log `/private/tmp/deluge-scale-menu-release.log`. No source edits during this build.
- [[2026-09-12]]: Release finished at `2026-09-12T23:35:21Z`, exit 0, elapsed 78 seconds, 159 build steps. ELF/BIN generated; linker reported its RWX-segment warning. Binary `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.bin` is 1,742,840 bytes, SHA-256 `bcf91abaf3f8bd09778c628a9b7b2a9110d5f1ddccf29231be03f875f5d7e566`. Embedded version uses the colocated parent identity `v1.3.1-dev-3d2dba951-dirty`; the actual feature checkpoint is `cd2701d953ba59cdd1ba4ea447f88c58fa5d44f9`. Use the binary hash to identify this build. No flashing or publication. Remaining checkboxes require hardware/full UI-Song behavior, not another compiler run.
