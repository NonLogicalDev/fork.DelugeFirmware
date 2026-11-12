---
description: "Keep source-matched playheads legible on full-screen pad and OLED waveform displays."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Improve waveform playhead visibility

## Purpose

Make the playhead easy to follow wherever one full-screen waveform represents the audio source currently playing. Dim RGB waveforms moderately and add a synchronized cursor to OLED waveform companions. This plan began on [[2026-08-30]].

## Current State

- Status: software-complete; physical Deluge validation remains.
- Result: `ns/bug-slicer-01`, change `utzutx`, commit `f8e3ff`, repairs exact Region and Manual/Lazy Slicer playhead ownership and stale exit cleanup. `nl/patch-oled-03`, change `sqppvq`, commit `7ce5b4`, dims only the four full-screen waveform bases, adds exact Sample Browser pad feedback, and adds bounded source-matched OLED playheads to Sample Marker Editor and both Slicer modes.
- Next action: physically verify pad and OLED legibility, source matching, reverse and loop movement, marker overlap, and cleanup before calling device behavior verified.

## Product integration

- **Existing model:** full-screen waveform surfaces render cached amplitude data; source-specific pad cursors are updated from the normal graphics routine. Local OLED companions are static and event-driven. Overview rows use separate timeline cursors.
- **User intent:** see the current source position immediately without the waveform visually swallowing it, including on the OLED waveform readout.
- **Integrated model:** every full-screen single-source waveform uses exact displayed-source identity, a moderately dim RGB waveform, and a high-contrast cursor. OLED waveform cursors reuse the same source position and viewport but update only when their projected pixel changes.
- **Architecture impact:** add a named full-screen waveform intensity, complete missing source-specific pad cursor coverage, add a bounded 20 Hz OLED cursor overlay, and keep multi-source overview rendering separate.

## Scope

### In scope

- Sample Marker Editor: dim pad waveform, preserve exact existing pad cursor, tighten source identity, and add OLED cursor.
- Region and Manual/Lazy Slicer: fix source selection, dim waveform pads, use the complete Region waveform height, preserve Manual controls, and add OLED cursor.
- Audio Clip View: dim the full-screen pad waveform while retaining its Clip-time cursor. Keep its existing OLED Clip ruler rather than add a second cursor.
- Sample Browser preview: dim the full-screen pad waveform and add an exact preview-source cursor. Keep its filename OLED unchanged.
- One-pixel OLED cursor drawn after waveform and markers, projected from raw sample position, capped at 20 Hz without a new timer.
- Focused policy/mapping tests, target compilation, complete configured host tests, adversarial review, and one logged Release build.

### Out of scope

- Session/Row, Grid, and Arranger thumbnails; recording and transition frames; Audio Recorder; 7SEG; a new playback engine; source scanning; firmware flashing; installation; publication; or upstream submission.

## Behavior contract

- A cursor appears only when the playing audio source exactly matches the source represented by the full-screen waveform.
- Full-screen RGB waveforms use a moderate editor intensity equivalent to 224 of 256. Markers, slice boundaries, undefined regions, controls, and cursor colors retain their established intensity and draw above the dimmed waveform.
- Sample Marker Editor follows the newest matching active voice. Audio Clip View follows only its displayed active Clip. Sample Browser follows only its displayed preview. Slicer follows browser preview in Region/new-Kit mode and the selected Sound in Manual/existing-Kit mode.
- Region Slicer maps its cursor over all eight waveform rows; Manual/Lazy Slicer maps it only over its waveform rows and leaves slice pads unchanged.
- Each OLED waveform cursor is a one-pixel inverted column drawn after waveform contours and markers. It becomes visible over both dark and lit pixels without erasing marker meaning.
- OLED movement is capped at 20 Hz through an existing graphics routine. Unchanged position produces no OLED update. Stop, release, source replacement, viewport change, and exit remove or remap the cursor once.
- OLED cursor rendering consumes cached waveform and marker state only. It performs no sample scan, cluster load, storage access, allocation, audio rendering, or new timer work.
- Reverse playback, loop wrap, scroll, zoom, start/end bounds, marker selection, slice controls, preview stop, and existing Clip ruler behavior remain intact.

## Completion Criteria

- [x] Every included full-screen pad waveform uses the moderate intensity while overlays retain their current strength.
- [x] Sample Marker Editor, both Slicer modes, Audio Clip View, and Sample Browser show only a source-matched pad cursor.
- [x] Sample Marker Editor and both Slicer modes show a synchronized, contrast-safe OLED cursor with bounded redraw work.
- [x] Region Slicer follows browser preview and Manual/existing-Kit Slicer follows its selected Sound without cross-source leakage.
- [x] Stopped, mismatched, offscreen, replaced, or exited sources leave no stale cursor in the reviewed software paths.
- [x] Focused checks, formatting, target ARM compilation, configured host tests, adversarial review, and one uninterrupted logged Release build pass.
- [ ] Physical Deluge checks confirm legibility, synchronization, marker overlap, reverse/loop behavior, and OLED stability before device verification is claimed.

## Blockers

- None for software implementation. Physical display and timing checks require the operator's Deluge.

## Artifacts

- **A1: Waveform playhead plan.** This file owns the surface matrix, behavior, and validation contract.
- **A2: Slicer source bugfix.** Planned `ns/bug-slicer-01` revision for exact Region/Manual source resolution.
- **A3: Waveform visibility feature.** Planned `nl/patch-oled-03` revision for pad intensity, missing pad cursor coverage, and OLED cursors.
- **A4: Repository contract.** Planned `AGENTS.local.md` guidance and self-contained deviation 25.

## Source entry points

- `src/deluge/gui/waveform/waveform_renderer.*`: full-screen waveform render intensity.
- `src/deluge/gui/ui/sample_marker_editor.cpp`: exact voice position, pad cursor, OLED waveform and markers.
- `src/deluge/gui/ui/slicer.cpp`: Region/Manual source ownership, row masks, pad cursor, and OLED waveform.
- `src/deluge/gui/views/audio_clip_view.cpp`: tinted pad waveform and Clip-time cursor.
- `src/deluge/gui/ui/browser/sample_browser.cpp`: preview waveform and exact preview voice.
- `src/deluge/gui/waveform/oled_waveform_renderer.*`: cached viewport mapping and OLED overlay policy.

## Decisions

- **Decision:** use a separate P47 rather than reopening the static OLED waveform plan.
	- **Reason:** P30 deliberately excludes playback-driven invalidation; this feature introduces bounded dynamic feedback across both pad and OLED displays.
- **Decision:** keep the confirmed upstream Slicer source defect in `ns/bug-slicer-01` and the visual feature in `nl/patch-oled-03`.
	- **Reason:** defect provenance and local feature ownership remain reviewable while both use stable component groups.
- **Decision:** dim only full-screen single-source waveforms.
	- **Reason:** Session and Arranger rows already own multi-source timeline cursors and should not become dimmer while stopped or inactive.
- **Decision:** invert one OLED column and redraw only changed columns.
	- **Reason:** monochrome inversion guarantees contrast with bounded work and no destructive gutter.

## Workers

- `/root`: active owner; plan, revision coordination, integration review, final validation, and operator communication.
- `/root/preview_trace`: active bugfix worker after plan activation; owns the bounded Slicer source-resolution correction.
- `/root/preview_review`: completed initial provenance and risk review; planned feature/re-review worker after the Slicer dependency is complete.
- `/root/owner_lookup`: completed fresh owner and duplicate lookup; confirmed P47 and plan-43 are unused.

## Implementation Steps

- [x] Inventory waveform surfaces, existing cursors, source identity, contrast, and refresh paths.
- [x] Lock the single-source surface matrix, moderate intensity, OLED contrast, and 20 Hz bound.
- [x] Create and validate `ns/bug-slicer-01` for exact Slicer source ownership.
- [x] Create `nl/patch-oled-03`, update deviation 25, and implement pad/OLED visibility.
- [x] Add focused source, mapping, contrast, and refresh-policy checks.
- [x] Run adversarial review and repair accepted findings in their owning revisions.
- [x] Run formatting, affected ARM compilation, configured host tests, and one logged Release build.
- [x] Leave flashing and physical verification to the operator.

## Work Log

- [x] [[2026-08-30]] - Traced the existing cursor and refined the request from missing playhead to cross-surface visibility.
	- **Evidence:** pad cursors already exist in Sample Marker Editor, Slicer, and Audio Clip View. Full-colour Audio Clip waveforms can exceed the fixed cursor intensity. OLED waveforms have no cursor, and Sample Browser has no cursor.
	- **Effect:** P47 owns moderate full-screen waveform intensity plus exact pad/OLED cursor coverage, not another playback mechanism.
	- **Next:** completed through the separate bug and feature revisions below.
- [x] [[2026-08-30]] - Repaired Slicer source ownership in `ns/bug-slicer-01`.
	- **Evidence:** Region/new-Kit follows only the active browser preview; Manual/Lazy and existing-Kit follow only the selected Sound. Exact displayed-sample identity, active sample-part eligibility, complete row masks, and both exit cleanup paths passed focused tests, ARM compilation, formatting, and two review passes.
	- **Effect:** Slicer no longer shows an unrelated Drum cursor or leaves its cursor over the destination view.
- [x] [[2026-08-30]] - Implemented and validated `nl/patch-oled-03`.
	- **Evidence:** focused playhead, OLED mapping, and Slicer tests passed; five affected ARM objects compiled; final adversarial review was clean; the reconfigured host suite passed 20 of 20; and the single 28-step Release build completed in 52.47 seconds.
	- **Effect:** the four full-screen waveform bases use the requested moderate level, overlays retain their strength, Browser gains exact pad feedback, and Sample Marker Editor plus Slicer gain one-pixel inverted OLED playheads with bounded refresh.

## Validation

- **Command or action:** read-only source and local-patch trace across full-screen waveform renderers, audio position sources, OLED cache mapping, and graphics refresh.
	- **Expected result:** identify every exact single-source surface and a bounded cursor implementation without misleading output or audio-path work.
	- **Observed result:** passed for design. Four full-screen surfaces are eligible; overview and non-waveform surfaces are excluded. One pre-existing Region Slicer source defect must be corrected first.
	- **Failure signals:** global transport position used for a raw sample waveform, unrelated voice ownership, full OLED repaint churn, shared thumbnail dimming, or storage/audio work during display refresh.
	- **Interpretation:** implementation proceeded through one dependency bugfix and one `oled` feature revision.
- **Command or action:** focused playhead, OLED waveform, and Slicer host checks; formatting; and five affected ARM Release objects.
	- **Expected result:** mapping, cadence, intensity policy, exact voice eligibility, and production units pass before full integration.
	- **Observed result:** passed: 5 playhead scenarios, 24 OLED renderer scenarios, 3 selected Slicer tests with 20 checks, formatter dry-run, and all five affected ARM objects.
	- **Failure signals:** wrong source, shadowed active voice, stale cursor, dimmed transition buffer, redraw churn, or compile failure.
	- **Interpretation:** no focused software blocker remained after accepted review repairs.
- **Command or action:** reconfigure, build, and run the complete host suite in the pinned Nix environment; log `/private/tmp/deluge-p47-waveform-playhead-host.log`.
	- **Expected result:** CMake discovers the new spec and every configured test passes.
	- **Observed result:** passed 20 of 20 tests in 1.35 seconds; the existing ETL version-warning remained non-fatal.
	- **Failure signals:** missing P47 test, configure failure, build failure, or any failed test.
	- **Interpretation:** the feature integrates with the complete configured host suite.
- **Command or action:** one uninterrupted `DBT_NO_SYNC=1 ./dbt build Release` in the pinned Nix environment; log `/private/tmp/deluge-p47-waveform-playhead-release.log`.
	- **Expected result:** one process links the Release ELF and generates the firmware artifacts.
	- **Observed result:** passed 28 steps in 52.47 seconds with only the existing RWX load-segment warning. `build/Release/deluge.bin` is 1,738,720 bytes with SHA-256 `252af78fd706562c379508e6330d390568f43e462ba98aa0118793e151135474`.
	- **Failure signals:** nonzero status, restarted build, input edit during build, missing artifact, or unexpected linker error.
	- **Interpretation:** the local Release firmware is ready for optional human-controlled device testing; it was not flashed.

## Unfinished Work

- [ ] Physical Deluge checks for pad/OLED legibility, exact source behavior, reverse and loop movement, marker overlap, source replacement, exit cleanup, and audio/display stability. No flash was performed.
