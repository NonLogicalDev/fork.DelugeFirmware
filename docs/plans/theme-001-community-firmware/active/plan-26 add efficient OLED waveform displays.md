---
description: "Prototype and add a low-cost OLED waveform companion to Deluge sample editing, including the Slicer."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add efficient OLED waveform displays

## Purpose

Show useful sample shape and edit position on OLED Deluges at a genuinely higher horizontal resolution than the 16 pad columns, without adding work to audio playback or continuously redrawing an unchanged screen. Include the Region and Manual/Lazy Slicer. This plan began on [[2026-08-24]].

## Scope

### In scope

- Map every current OLED sample-bound editing surface and the Slicer's Region and Manual/Lazy modes.
- Render a compile-time-adjustable number of independently measured min/max peak buckets across the 128-pixel OLED. Default to 128 buckets, one measured envelope per physical column and eight times the pad-grid resolution; require the configured count to divide 128 evenly and support 16, 32, 64, or 128 without changing renderer logic.
- Keep the higher-resolution fixed-size cache local to sample editing, populate it outside OLED drawing, and use integer math plus event-driven redraws.
- Show the visible sample region plus the bounds or slice position needed to understand the current edit.
- Keep a non-empty waveform visible throughout the supported zoom range for long recorded samples; a viewport or cache transition must not blank valid measured data.
- Measure the additional cache and peak-analysis work well enough to reject per-clip memory growth, OLED-time sample reads, heap allocation, animation, timers, or unchanged redraws.
- Integrate only the surfaces where the waveform adds useful editing context.

### Out of scope

- Audio-engine, playback, pad-waveform layout or interaction, color, slicing-algorithm, or saved-project changes.
- Background waveform analysis, continuous playhead animation, firmware flashing, installation, publication, or upstream submission.
- Pre-existing shared waveform address and half-open reversal defects, which are owned by P37 and P38 even though the OLED feature depends on their corrected behavior.

## Current State

- Status: software-complete with the full 128-pixel display width as the default measured fidelity in `nl/patch-oled-01`; completed P37 and P38 still own the two pre-existing shared waveform repairs.
- Next action: confirm readability, control feel, and practical power behavior on a physical OLED Deluge. The agent will not flash the device.

## Completion Criteria

- [x] Source mapping identifies the exact OLED surfaces, available cached waveform data, redraw triggers, and audio/storage boundaries.
- [x] The prototype demonstrates a fixed-cost render path with no heap allocation, sample-file read, decoder work, or audio-thread call.
- [x] Sample-bound scrolling and edits draw a waveform with clear current bounds on OLED.
- [x] Region and Manual/Lazy Slicer modes draw the waveform and their relevant divisions or selected slice position.
- [x] Unchanged state does not schedule repeated OLED redraws, and no new timer or animation loop is added.
- [x] Focused host tests or a deterministic renderer harness cover scaling and marker bounds; the configured host suite and Nix Release build pass.
- [x] `DEVIATIONS.md`, `AGENTS.local.md`, owner state, and the separated Jujutsu revisions describe their final behavior and ownership. Nothing is flashed or published.
- [x] The OLED contour defaults to 128 independently measured min/max peak buckets across the visible sample region, with each bucket mapped to exactly one physical OLED column. The compile-time count remains constrained to an exact divisor of 128 and supports 16, 32, 64, and 128 without changing renderer logic; repeated ownership at viewports shorter than the configured bucket count is explicit rather than presented as new source detail.
- [x] The higher-resolution cache is editor-local, fixed-size, refreshed only with the normal sample-editing render lifecycle, and never populated by `renderOLED()`.
- [x] Focused tests, independent review, the configured host suite, and a new Nix Release build validate the 128-bucket default, alternate compile-time geometry, and its memory/code-size cost.
- [x] Long recorded samples retain valid waveform data across supported deterministic zoom and address-boundary cases, including changes that invalidate or partially populate the prior cache.
- [ ] A person confirms readability, control feel, and practical power behavior on a physical OLED Deluge.

## Blockers

None. Hardware power draw and final OLED readability cannot be measured by the agent, so local performance evidence must be paired with later physical confirmation.

## Artifacts

- **Artifact:** OLED waveform prototype and renderer.
	- **Type:** Firmware source and focused validation.
	- **State:** Reopened to make the already-supported 128-bucket geometry the production default in the same feature-only revision; physical OLED and power checks remain.
	- **Contents:** Project 128 independently measured min/max peak buckets across the 128-pixel OLED as sparse upper and lower contours, one bucket per physical column, then overlay caller-owned bounds or slice markers. Keep the renderer valid for compile-time counts that divide the display width.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/waveform/oled_waveform_render_data.h`, `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/waveform/oled_waveform_renderer.h`, `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/ui/sample_marker_editor.cpp`, and `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/gui/ui/slicer.cpp`.
	- **Build artifact:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.bin`, SHA-256 `9fde300a4d3631f7d26e3237bb13f3aaba9138ce411fe093d7b78310ae58d266`; matching ELF SHA-256 `50fab8b9fc713ad0653a546d473bcd20acfd7d2ef17c8b4c47c76dd9a5a96cbb`.
	- **SCM surface:** Jujutsu change `pvqpsstyvuynpuxtrlxmwlukrwrqntlm`, commit `da52ea71ad795da68723ebdddeb543db8884c152`, bookmark `nl/patch-oled-01`. Completed P37 `ns/bug-waveform-01` and P38 `ns/bug-waveform-02` precede it as separate bug revisions.
- **Artifact:** Self-contained deviation contract.
	- **Type:** Firmware behavior document.
	- **State:** Complete for the implemented local behavior.
	- **Contents:** OLED waveform behavior, performance limits, compatibility, and verification.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.

## Decisions

- **Decision:** Prototype a shared pure renderer before wiring individual screens.
	- **Rationale:** The source map confirmed that one renderer can consume a prepared fixed peak cache. It never calls the peak finder, sample decoder, cluster loader, or audio engine.
- **Decision:** Treat battery efficiency as avoided work, not an unverifiable power claim.
	- **Rationale:** The agent can verify allocations, reads, call paths, redraw triggers, and fixed iteration counts. Only a physical device can verify actual battery life.
- **Decision:** Limit the feature to `SampleMarkerEditor` and `Slicer` Region and Manual/Lazy modes.
	- **Rationale:** These are the OLED editors where sample shape changes an editing decision. Sample Browser, idle Audio Clip view, Session, Arranger, recording, and playhead animation would add work without serving the same task.
- **Decision:** Default to 128 measured min/max envelope buckets, one per physical OLED column, and keep the count adjustable at compile time without widening every Audio Clip's waveform cache.
	- **Rationale:** The user prefers maximum display-width fidelity over the prior 64-bucket analysis/cache compromise. The configured count remains at least the 16 pad columns, no greater than 128, and an exact divisor of the display width; this admits exactly 16, 32, 64, and 128. Each bucket measures its own sample interval when the viewport contains enough samples. At tighter viewports, adjacent buckets may explicitly repeat the sample they own because no additional source samples exist. OLED drawing consumes the cache only; required peak analysis remains in the existing sample-editing render lifecycle and must be measured.
- **Decision:** On the two OLED editing surfaces, measure the configured high-resolution cache once and aggregate adjacent buckets into the navigator's existing 16 conceptual pad columns.
	- **Rationale:** Running the established 16-bin collector and then a separate 128-bin collector would require 144 bucket investigations per changed viewport. At the 128-bucket default, folding each group of eight measured envelopes into one pad column caps successful population at 128 investigations and keeps OLED and pads coherent. This does not widen or alter any Audio Clip cache or path; only the shared sample-editing navigator uses the finer source data. Pending children keep their aggregate pad column retryable, all-beyond children produce a beyond column, and investigated children contribute exact min/max extrema.
- **Decision:** Retain partial OLED results across temporary load failures inside `nl/patch-oled-01`.
	- **Rationale:** A failed cluster load leaves only that bucket pending; already investigated OLED buckets remain drawable and the editing render lifecycle retries the incomplete cache. This retry and redraw behavior belongs to the new OLED cache.
- **Decision:** Separate pre-existing shared waveform defects from the OLED feature.
	- **Rationale:** P37 owns the signed-32-bit sample and byte narrowing in `ns/bug-waveform-01`; P38 owns the reversed half-open range defect in `ns/bug-waveform-02`. OLED review exposed both defects but did not create them.
- **Decision:** Make Slicer own its OLED page and keep updates event-driven.
	- **Rationale:** A full page is clearer and cheaper than stacking a modal over the Browser. Only entry, mode/count, boundary, selection, and successful viewport changes may invalidate OLED; audition, playhead, blink, and graphics timers may not.

## Product Integration

- Existing model: sample editors own their navigation and marker state; OLED render methods currently present local text or controls.
- Real intent: see sample shape and the edited region without moving attention back to the pad grid.
- Integrated model: the shared sample-editing navigator owns one fixed cache containing a compile-time bucket count of measured min/max envelopes, defaulting to 128. `SampleMarkerEditor` and `Slicer` consume it with their own marker state; the OLED renderer never owns playback, navigation, or sample loading.
- Pieces that may change: a shared OLED waveform helper, `SampleMarkerEditor`, `Slicer`, focused tests, and contracts.
- Architecture limit: no audio-path work, new background job, persistent waveform copy, or unconditional graphics-loop redraw.

## Implementation Steps

- [x] Trace the OLED, sample navigation, waveform cache, and redraw paths in parallel.
- [x] Build and measure the smallest renderer prototype against representative empty, short, long, zoomed, and bounded inputs.
- [x] Integrate the accepted prototype into sample-bound editing and both Slicer modes.
- [x] Run adversarial performance and product review; remove any renderer state or redraw path that does not earn its cost.
- [x] Format, test, build, record the Jujutsu bookmark, and leave physical verification pending.
- [x] Trace the narrowest way to collect 64 min/max envelopes by default during the existing sample-editing render lifecycle without widening per-clip caches, while keeping the count compile-time adjustable.
- [x] Replace the stretched 16-bin contour with measured bucket data at the configured resolution, then adversarially review analysis frequency, storage/audio boundaries, cache cost, redraw behavior, and alternate valid geometries.
- [x] Separate P37 and P38, then re-run focused tests, the configured host suite, Release build, size comparison, and independent review with `nl/patch-oled-01` containing only the OLED feature.
- [x] Set the production default to 128 buckets, update the self-contained contract and default-geometry tests, and fold the correction into the existing `nl/patch-oled-01` revision.
- [x] Re-run focused tests, 16/32/64/128 compile checks, independent review, the configured host suite, formatter, size/cache comparison, and one logged Release build.

## Workers

- `/root`: active owner; completed plan state, Jujutsu integration, the final software gate, owner records, and user communication; owns later physical verification.
- `/root/waveform_full_width`: completed the 128-bucket default, ceiling-inverse range partition, 24-case count-generic spec, 16/32/64/128 executable matrix, formatter, and self-contained deviation contract before folding.
- `/root/waveform_full_width_review`: completed three adversarial review cycles; found the front-loaded pad mismatch, missing alternate-count execution, incomplete tight-zoom contract, and floor-versus-ceiling marker mismatch, then passed the corrected final draft with no blockers.

## Work Log

- [x] [[2026-08-24]]@13:03 - Opened P30 for one OLED waveform feature spanning sample-bound editing and the Slicer.
	- **Evidence:** The user requested a prototype and battery/performance-efficient implementation that includes the sample chopper.
	- **Effect:** OLED waveform work has one plan and must prove avoided work before source integration.
	- **Next:** Map the existing render and waveform-data paths with focused workers.
- [x] [[2026-08-24]]@13:03 - Assigned three read-only source investigations before choosing the prototype.
	- **Evidence:** `/root/oled_waveform_data` owns cached waveform data; `/root/oled_waveform_surfaces` owns product surfaces; `/root/looper_core` was repurposed to own OLED scheduling, tests, and performance hazards.
	- **Effect:** The source map can proceed in parallel without overlapping edits.
	- **Next:** Integrate their evidence into the renderer and surface decision before opening a source revision.
- [x] [[2026-08-24]]@13:08 - Accepted the shared 16-bin contour renderer and two-surface product model.
	- **Evidence:** All three investigations independently found that pad rendering precedes OLED rendering and leaves fresh 16-bin peak data; a new OLED peak scan can load storage clusters and call the audio engine, while an OLED frame transfer always sends 768 bytes.
	- **Effect:** P30 excludes new peak analysis, a wider per-clip cache, animation, idle screens, and graphics-loop invalidation. The prototype owns only `SampleMarkerEditor` and `Slicer` OLED pages.
	- **Next:** Save a baseline ELF, open `nl/patch-oled-01`, implement the pure renderer and focused test, then wire the two editors.
- [x] [[2026-08-24]]@13:09 - Preserved the current Release ELF and opened the isolated OLED feature revision.
	- **Evidence:** Baseline `/tmp/deluge-oled-baseline.elf` has text 1,634,488 bytes, data 67,828 bytes, and BSS 362,952 bytes. Jujutsu change `pvqpsstyvuynpuxtrlxmwlukrwrqntlm` is an empty child of `nl/patch-song-grid-02`.
	- **Effect:** Firmware-size comparison has a stable baseline and P30 no longer shares a revision with earlier features.
	- **Next:** `/root/oled_waveform_data` implements the pure helper and tests while `/root/oled_waveform_surfaces` integrates the two editor pages against that API.
- [x] [[2026-08-24]]@13:20 - Integrated one cached contour renderer across Sample Marker Editor and both Slicer modes.
	- **Evidence:** `SampleMarkerEditor` now shows its compact marker/value header, sparse contour, full selected bound, and short secondary marker ticks. Slicer owns an OLED page: Region shows count and readable equal divisions; Manual/Lazy shows slice index, count, start, all cuts, and the selected start/end bounds.
	- **Effect:** The two accepted editing surfaces share one implementation and no new sample-analysis or display-refresh system.
	- **Next:** Review fixed work, invalidation triggers, coordinate edges, and physical-power wording before final validation.
- [x] [[2026-08-24]]@13:30 - Removed avoidable CPU work and byte-identical OLED transfers after adversarial review.
	- **Evidence:** The contour now performs one Q40 scale division per non-flat frame, then exact multiply/shift projection for up to 32 contour lines and 256 waveform pixels. One viewport scale division makes subsequent marker projections division-free. Region setup advances an exact quotient/remainder recurrence and considers at most 64 two-pixel-spaced candidates. Clamped Manual movement, ineffective Manual Select turns, repeat audition of the selected slice, failed slice capture, and hidden-only transpose reset no longer invalidate OLED.
	- **Effect:** Renderer cost is fixed by display width, static views do not retransmit unchanged 768-byte frames, and actual battery life remains an explicit physical measurement rather than a software claim.
	- **Next:** Rebuild, compare ELF size, and request a clean re-review.
- [x] [[2026-08-24]]@13:36 - Completed local validation and recorded the component revision.
	- **Evidence:** Thirteen focused waveform cases pass inside the configured 8-of-8 CTest suite. The Nix Release build linked `Deluge.bin` with SHA-256 `52e4e95fe8e871d9fce94e049b8c4536935ef103b8cadc8d4a7a68b32ea13214`. Final text is 1,635,884 bytes, 1,396 bytes over baseline; data remains 67,828 bytes and BSS remains 362,952 bytes. Independent re-review found no remaining actionable issue. Jujutsu bookmark `nl/patch-oled-01` points to `0b5edf92fb9e`.
	- **Effect:** Local source, contract, tests, build, static-memory comparison, and revision bookkeeping are complete without Git, flashing, installation, publication, or upstream submission.
	- **Next:** Check readability, control feel, and actual power behavior on a physical OLED Deluge before choosing whether to flash.
- [x] [[2026-08-24]]@16:56 - Reopened P30 for genuine higher-resolution waveform data.
	- **Evidence:** The current renderer repeats each of 16 pad peak buckets across eight OLED pixels. The user requested a resolution tied to the actual display rather than an arbitrary multiplier.
	- **Effect:** Interpolation or wider line drawing no longer satisfies P30. The accepted target is 128 independently measured min/max buckets, one per OLED column, in an editor-local cache folded into `nl/patch-oled-01` without a cancellation patch.
	- **Next:** Compare the available peak-finding and cache ownership seams before choosing the lowest-cost 128-bin population path.
- [x] [[2026-08-24]]@16:58 - Added the long-recording zoom dropout to the investigation.
	- **Evidence:** On an older build, a long recorded sample lost its OLED waveform at a particular zoom level.
	- **Effect:** P30 must trace cache status, amplitude scaling, viewport limits, and zoom invalidation rather than assuming the blank display is only coarse resolution. Valid measured data must survive every supported zoom transition.
	- **Next:** Reproduce the responsible source path with large-span deterministic cases and classify whether the cause belongs to upstream waveform behavior or the local OLED feature.
- [x] [[2026-08-24]]@17:00 - Chose a 64-bucket default with compile-time-adjustable display geometry.
	- **Evidence:** A one-bin-per-pixel design doubles peak-analysis and cache work relative to 64 bins, while 64 still improves the original 16-bin contour from eight-pixel blocks to two-pixel blocks. At viewports narrower than the configured bucket count, distinct source samples do not exist for every bucket.
	- **Effect:** The accepted default is 64 independently measured min/max buckets across 128 OLED columns. The count must divide 128 evenly and renderer mapping must also support 16, 32, and 128. Repeated sample ownership at very tight zoom is allowed and must not be described as independent source detail.
	- **Next:** Finish the source-path and long-recording investigations, then implement the smallest editor-local cache that satisfies the adjustable geometry.
- [x] [[2026-08-24]]@17:04 - Traced the exact long-recording failure and accepted the editor-local population path.
	- **Evidence:** `WaveformRenderer::findPeaksPerCol()` computes signed 32-bit sample and byte bounds even though its viewport inputs are 64-bit and the recorder supports files through 4 GiB. Byte offsets wrap at 2 GiB, producing negative or otherwise invalid cluster math when a zoomed-out viewport reaches later audio. The shared navigator is the only sample-editing owner, so a 64-bucket cache adds 592 bytes there and adds nothing to each Audio Clip.
	- **Effect:** Peak-bound arithmetic will be widened at the shared collector rather than patched in OLED drawing. On the accepted OLED editing surfaces, the new cache is measured once and folded into the existing 16 pad columns instead of running two complete collectors; Audio Clip and non-OLED paths remain unchanged. Direct zoom explicitly refreshes it before the OLED is dirtied.
	- **Next:** Implement the generic fixed-size cache, partial-result retry, and exact bucket partitioning in `nl/patch-oled-01`; keep the pre-existing collector address repair and its boundary tests separable.
- [x] [[2026-08-24]]@17:23 - Validated the adjustable 64-bucket implementation and waveform repairs in one combined change before revision separation.
	- **Evidence:** `WaveformBasicNavigator` owns a fixed 592-byte default cache. Main-image renders in the two OLED editors populate at most 64 buckets and aggregate them into the established 16 pad columns without a second analysis pass. Offscreen transition buffers retain the legacy 16-column path. Complete cache hits perform only key/provenance checks; partial loads retain successful buckets and requeue only pending work. The shared collector keeps sample coordinates signed 64-bit and valid byte and cluster positions unsigned 64-bit through range validation.
	- **Effect:** OLED drawing remains pure and fixed-work, Audio Clip storage is unchanged, and direct successful zoom refreshes before OLED invalidation. P37 and P38 now own the two pre-existing shared waveform repairs that were temporarily mixed into this change.
	- **Next:** Preserve this combined evidence, then extract `ns/bug-waveform-01` and `ns/bug-waveform-02` before treating `nl/patch-oled-01` as final.
- [x] [[2026-08-24]]@17:31 - Completed the local software gate for the combined 64-bucket source.
	- **Evidence:** Clang-format passed all changed C++ files. The focused OLED spec passed 24 cases and the configured CTest suite passed 8 of 8 targets. Production-header compile-only checks with warnings as errors passed at 16, 32, and 128 buckets. Ninja cleaned 506 Release outputs and rebuilt all 506 steps successfully into `build/Release/deluge.elf` and `build/Release/deluge.bin`. Independent ARM GCC and LLVM review found no P30 blocker.
	- **Effect:** The pre-separation artifact is Jujutsu change `pvqpsstyvuynpuxtrlxmwlukrwrqntlm`, commit `4625a88ee23016f07c4c53b988dce16c8e2f4e55`, bookmark `nl/patch-oled-01`. It proves the combined behavior but not the final revision boundaries. No Git, flashing, installation, publication, or network action was used.
	- **Next:** Leave P30 active through P37/P38 separation and post-separation validation, then perform the physical OLED checks.
- [x] [[2026-08-24]] - Completed revision separation and the post-separation software gate.
	- **Evidence:** P37 is `ns/bug-waveform-01` at `026a809d9b8fd167badc979422413c613d182d6f`; P38 is `ns/bug-waveform-02` at `c4bfc3d7a1157130b868aa08bfedb97c1b256c5e`; feature-only P30 is `nl/patch-oled-01` at `da52ea71ad795da68723ebdddeb543db8884c152`. Clang-format passed all feature files, the configured CTest suite passed 10 of 10 targets, 16/32/128 compile-only checks passed with warnings as errors, and the clean 506-step Release build completed.
	- **Effect:** Each pre-existing waveform defect is independently reviewable and `nl/patch-oled-01` contains only the OLED feature and corrections to that feature. The post-split BIN SHA-256 is `9fde300a4d3631f7d26e3237bb13f3aaba9138ce411fe093d7b78310ae58d266`.
	- **Next:** Leave P30 active only for physical OLED readability, control-feel, and practical power validation.
- [x] [[2026-08-24]]@21:09 - Reopened P30 for full-display-width default fidelity.
	- **Evidence:** The renderer and compile-time geometry already support 128 buckets, but the production default remains 64 and maps each measured envelope to two physical columns. The user requested full OLED pixel-width fidelity.
	- **Effect:** The accepted production default is now 128 independently measured min/max buckets, one per physical column. The 16/32/64 alternatives, editor-local ownership, event-driven population, and all audio/storage/render boundaries remain unchanged.
	- **Next:** Reconcile the current constant, default-specific tests, cache size, and deviation contract before editing `nl/patch-oled-01`.
- [x] [[2026-08-24]]@21:13 - Completed the read-only 128-bucket reconciliation.
	- **Evidence:** The production mismatch appeared to be one default macro in `oled_waveform_render_data.h`; existing validity, duplicate-range reuse, pad aggregation, and renderer geometry accept 128. Default-specific expectations live in the waveform spec and Deviation 17. The single editor-local cache grows from 592 to 1,168 bytes, predicting +576 BSS with no per-Clip or heap growth. The later adversarial review disproved the assumption that the existing remainder partition remains pad-aligned at 128.
	- **Effect:** The constant, default-specific cases, and contract remain required, but the partition and its focused proof also need correction before the draft can be folded. The software gate must exercise production 128 plus compile-only 16/32/64/128 geometry, cache size, host tests, formatting, review, and one logged Release build.
	- **Next:** Implement in a temporary child of `nl/patch-oled-02`, validate it, then fold the three-path correction into `nl/patch-oled-01` and verify the rebased ruler revision.
- [x] [[2026-08-24]]@21:19 - Correct the 128-bucket partition so OLED buckets, pad columns, and marker coordinates share the same boundaries.
	- **Evidence:** At `xZoom = 9`, front-loading the 16 remainder samples makes the first two eight-bucket groups cover 16 samples each while later groups cover 8; each pad column covers exactly 9. The prior generalized test proved only contiguity and total coverage, so it did not detect the coordinate mismatch.
	- **Effect:** The draft is blocked from folding. Partition boundaries must be derived proportionally across the whole viewport so every group of `bucketCount / 16` buckets covers exactly the corresponding pad interval, including remainder cases. Focused tests must directly assert per-pad boundary equality rather than only bucket-width counts.
	- **Next:** Repair the temporary change, rerun focused and alternate-geometry checks, then have the adversarial reviewer inspect the corrected draft.
- [x] [[2026-08-24]]@21:20 - Make alternate geometry and short-viewport behavior mechanically provable.
	- **Evidence:** The focused spec hard-requires the production 128 count and 1,168-byte cache, so compiling only the renderer header at 16/32/64 does not exercise range partitioning, pad aggregation, or pixel placement at those counts. The contract also calls all 128 default buckets independently measured even when fewer than 128 source samples are visible and adjacent buckets must deliberately share a source sample.
	- **Effect:** Alternate-count validation needs an instantiable geometry-level test or equivalent executable proof at 16, 32, 64, and 128. Deviation 17 must state that each bucket is independently addressed when source detail exists and that duplicate ownership is required at shorter viewports rather than claiming nonexistent detail.
	- **Next:** Add the narrowest compile/run proof for partition, aggregation, and pixel geometry at all four supported counts; update the self-contained contract; include both in final re-review.
- [x] [[2026-08-24]]@21:21 - Accepted the bounded OLED/UI cost of full-width fidelity.
	- **Evidence:** Moving from 64 to 128 cached buckets doubles the contour loop from 64 to 128 iterations and can raise horizontal-line calls from 128 to 256. Pixel writes remain capped at 256 because each default bucket occupies one column rather than two. Cache invalidation, pending-count, and pad-aggregation loops also double, but no new real-time audio callback, decoding, storage access, allocation, or OLED-triggered sample analysis is introduced.
	- **Effect:** The performance contract forbids expansion into the real-time audio and storage paths, not the requested bounded cached UI work. Final validation must retain the 256-pixel-write bound, prove event-driven population, and measure the fixed cache and Release size cost.
- [x] [[2026-08-24]]@21:28 - Make each OLED range the exact inverse of sample-to-pixel marker projection.
	- **Evidence:** The first repair made every OLED bucket group match its pad interval, but used floor-based individual boundaries. At a 144-sample viewport, sample offset 1 projects to OLED column 0 while the floor partition assigns it to bucket 1, producing a one-column waveform/marker mismatch that the pad-boundary test cannot see.
	- **Effect:** Non-tight bucket boundaries must use the ceiling inverse of `floor(sampleOffset * bucketCount / viewportSpan)`, while tight viewports retain deliberate one-sample duplicate ownership. Tests must compare representative sample positions against their owning bucket and restore ceiling-based large-span expectations. Pad-group equivalence must continue to pass at all four supported counts.
	- **Next:** Repair the temporary change, add direct marker-to-bucket ownership cases, explicitly require duplicate-range result reuse in Deviation 17, then rerun the full focused geometry matrix and final re-review.
- [x] [[2026-08-24]]@21:38 - Folded and validated the full-width production default.
	- **Evidence:** `nl/patch-oled-01` is Jujutsu change `pvqpsstyvuynpuxtrlxmwlukrwrqntlm`, current rebased commit `d0fd68f3712c21a5bd47ea8bdf513ece5f4f5659`; rebased child `nl/patch-oled-02` is change `pkolkoympqyoqzsntmuwqknrxzoyvslk`, current commit `1d7c88b1d2c276c87c2288c23a826d4aa71a667c`, with no conflicts and an empty clean working change above it. The production focused spec passes 24 cases. Actual compiled 16/32/64/128 variants each pass the same focused CTest, the configured host suite passes 11 of 11, formatter passes, and final adversarial review passes with no blockers. The logged incremental Release build completed in 47.33 seconds.
	- **Effect:** The default is one measured bucket per OLED column. Each pad group covers its exact pad interval; non-tight sample ownership is the ceiling inverse of marker projection; tight duplicate ranges reuse one collected peak result. The singleton cache is 1,168 bytes and BSS grew by the predicted 576 bytes. Against the corrected P40 baseline, text grew 164 bytes, data stayed unchanged, and total ELF sections grew 740 bytes.
	- **Next:** Leave only physical OLED readability, control-feel, and practical-power checks pending; do not flash or publish.

## Validation

- **Command or action:** Source-path and call-frequency review.
	- **Expected result:** No proposed render work is reachable from audio processing or repeated without a state change.
	- **Observed result:** Passed. On the two physical-OLED main-image editor paths, one configured high-resolution pass populates the navigator cache and folds it into the existing pad columns. A current complete cache returns after constant-time key and provenance checks. `renderOLED()` reads fixed arrays and marker state only; sample, storage, decoding, allocation, timers, and audio-engine calls remain outside it. Offscreen transition buffers do not populate or consume the OLED cache-change edge.
	- **Failure signals:** Sample-file reads, decoding, allocation, timers, graphics-loop invalidation, or unbounded iteration in the OLED path.
	- **Interpretation:** This establishes the architectural work boundary and the 64-measurement maximum for a newly successful default viewport. It does not establish physical battery life.
- **Command or action:** Focused renderer tests, configured host CTest suite, and local Nix Release build.
	- **Expected result:** Deterministic scaling and markers, all tests passing, and a linked firmware image.
	- **Observed result:** Passed after separation. The direct spec runner passed 22 OLED cases; P37's 2 large-address cases and P38's 1 reversed-range case passed separately. The configured CTest suite passed 10 of 10 targets. The clean 506-step Release build is recorded in `/tmp/deluge-oled-postsplit-build.log`. `build/Release/deluge.bin` has SHA-256 `9fde300a4d3631f7d26e3237bb13f3aaba9138ce411fe093d7b78310ae58d266`; `build/Release/deluge.elf` has SHA-256 `50fab8b9fc713ad0653a546d473bcd20acfd7d2ef17c8b4c47c76dd9a5a96cbb`.
	- **Failure signals:** Marker overflow, invalid sample ranges, regression failures, compile or link errors, or firmware-size growth that the feature does not justify.
	- **Interpretation:** Final sections are text 1,640,952 bytes, data 67,828 bytes, BSS 363,560 bytes, and 2,072,340 bytes total. Against `/tmp/deluge-oled-baseline.elf`, this is +6,464 text, +0 data, +608 BSS, and +7,072 total. Against the superseded `/tmp/deluge-oled-16bin.elf`, it is +5,068 text, +0 data, +608 BSS, and +5,676 total. OLED readability and power impact remain physical checks.
- **Command or action:** Historical 64-bucket adjustable-geometry and population-path validation.
	- **Expected result:** The default build uses 64 independently measured min/max peak buckets across the current 128-pixel viewport and maps each bucket to two OLED columns. Compile-time checks accept supported divisors through 128, mapping remains exact, and no sample analysis or storage access occurs from OLED drawing.
	- **Observed result:** Passed after separation. The default 64-bucket feature passed the full host and Release gates. The production header passed compile-only checks with `-Werror` at 16, 32, and 128 buckets. The feature-focused cases cover accepted and rejected counts, two-column default rendering, quotient/remainder partitioning, repeated ownership at tight zoom, folding extrema and statuses, partial retry, cache-hit signaling, and fixed render work. P37 separately covers signed-32-bit, 2 GiB, and 4 GiB boundaries; P38 separately covers corrected reversed half-open ranges.
	- **Failure signals:** Reusing only the 16 pad peaks, interpolating between them and calling that resolution, widening every Audio Clip cache, peak finding from `renderOLED()`, background refresh, or repeated analysis while the viewport is unchanged.
	- **Interpretation:** Passing establishes the selected performance/detail compromise, alternate geometry, and event-driven cache lifecycle in the feature-only revision. P37 and P38 independently establish the generic address and range behavior. Actual readability and power remain physical checks.
- **Command or action:** Final 128-bucket production geometry, alternate-count execution, and Release gate.
	- **Expected result:** The ordinary build uses 128 buckets and one physical OLED column per bucket; sample ownership agrees with marker projection, every bucket group agrees with its pad interval, tight duplicates reuse one result, alternate 16/32/64/128 builds execute the same behavior suite, and the fixed cache costs exactly 576 additional BSS bytes over the 64-bucket build.
	- **Observed result:** Passed. The ordinary focused spec passes 24 cases and retains compile-time assertions for 128 buckets, one pixel per bucket, and a 1,168-byte render-data cache. Explicit 16/32/64/128 builds each compiled 20 targets and passed the same focused CTest and direct spec. The configured host suite passed 11 of 11 in 1.33 seconds. Clang-format dry-run passed both changed C++ files. Three adversarial review cycles passed the corrected final draft. The logged Nix Release build in `/tmp/deluge-release-waveform-128.log` completed in 47.33 seconds and produced text 1,644,376 bytes, data 67,828 bytes, BSS 364,296 bytes, and 2,076,500 bytes total. `deluge.bin` SHA-256 is `40b3a7853174177f68426bee3ca59216693dbfbf86cd39ea20f81409701f9112`; `deluge.elf` SHA-256 is `8d13dc675ebb6d0aba7ddae83a87c0365b4c4786ac3c38ddf1bea68a6ae419ce`.
	- **Failure signals:** A sample peak and its marker land in different columns, a pad aggregate covers a different half-open range than its child buckets, tight duplicates repeat analysis or invent detail, any alternate count fails the executable spec, UI rendering reaches storage or audio work, or measured BSS growth exceeds the fixed cache delta without explanation.
	- **Interpretation:** Against the corrected P40 baseline of text 1,644,212, data 67,828, BSS 363,720, and total 2,075,760, this change is +164 text, +0 data, +576 BSS, and +740 total. It proves the requested software fidelity and bounded cost; OLED readability and battery impact remain physical checks.
- **Command or action:** Formatting and independent analyzer review.
	- **Expected result:** Every changed C++ file matches repository formatting, and an independent worker finds no blocking reachable defect in the production paths.
	- **Observed result:** Passed after separation. Clang-format dry-run passed all feature files. ARM GCC 14.2.1 `-fanalyzer -c` produced zero diagnostics for the renderer, navigator, sample-marker editor, and Slicer; logs are `/tmp/deluge-static-analysis-ab821z/gcc-analyzer-postsplit-*.log`. LLVM 21.1.8 reruns in `/tmp/deluge-static-analysis-ab821z/clang-tidy-postsplit-*.log` reproduced only the previously reviewed bounded conversion and API-shape warnings, with no new reachable finding.
	- **Failure signals:** Format drift, a reachable analyzer finding, or an analyzer limitation without a complementary production compiler pass.
	- **Interpretation:** No source-level OLED blocker remains after separation. The reversed-range regression lives with completed P38 because both analyzers missed that pre-fix out-of-bounds defect.

## Unfinished Work

- [ ] Confirm OLED readability and control feel on the physical Deluge before any flashing decision.
- [ ] Observe practical power behavior on the physical Deluge; do not infer battery impact from software structure alone.
