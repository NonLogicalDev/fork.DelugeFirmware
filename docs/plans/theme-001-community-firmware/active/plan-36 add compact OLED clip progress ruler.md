---
description: "Add a compact beat-marked OLED ruler that follows the current Clip playhead and viewport."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add compact OLED Clip progress ruler

## Purpose

Show where the current Clip is playing or recording across its whole loop without taking useful OLED space. A tiny ruler should hug the top edge, mark musical time, follow the playhead across the full Clip, and show which portion of that Clip is currently visible on the pad matrix. This plan began on [[2026-08-24]].

## Product integration

- Existing model: Clip views already know the whole Clip length, visible pad-matrix range, playback position, and when the pad playhead needs repainting, but the OLED gives no persistent whole-Clip progress cue.
- User intent: glance at the OLED to understand progress through the entire Clip and where the currently visible or edited pad-matrix range sits inside it.
- Integrated model: one shared Clip-view overlay maps the whole Clip loop to the OLED width and draws the visible viewport as a separate selection span over that fixed domain. Horizontal scroll or zoom moves or resizes only that selection; it never rescales or resets the playhead.
- Architecture impact: keep geometry and rendering shared; let each supported Clip view supply its existing timeline state. Add no audio-path drawing, heap allocation, or independent animation timer.

## Scope

### In scope

- Normal Instrument Clip View for Synth, Kit, MIDI Out, and CV, normal Audio Clip View, and every non-Arranger Clip Automation surface: Instrument and Audio Automation Overview, parameter automation, and note Velocity editing.
- Keyboard View's complete `KeyboardScreen` surface: Isomorphic, In Key, Piano, Chord, Chord Library, Velocity Drums, and Norns layouts. The operator's "Pad View" means Velocity Drums; the ordinary step-sequencer pad matrix is already covered by Instrument Clip View.
- Automation's own full-canvas redraws must repaint the ruler, and its existing graphics callback must use the same shared 20 Hz ruler cache and cadence.
- Beat and bar marks derived from the Clip's musical timebase across its complete loop.
- A compact playhead/progress indicator whose full OLED traversal equals one complete Clip loop.
- A distinct viewport-selection span showing the portion currently visible on the 16-pad matrix.
- Immediate correction after horizontal scroll, zoom, Clip-length change, Clip change, playback start/stop, and record-state change.
- Focused geometry/render tests, configured host tests, Release build, source review, physical-screen checklist, and a self-contained `DEVIATIONS.md` contract.

### Out of scope

- 7SEG displays, Performance View, Song Grid/Row views, Arranger and Arranger Automation, Sound Editor menus, Sample Browser, Slicer, Sample Marker Editor, or idle background animation.
- A transport timeline for the whole Song, waveform rendering, new timers, audio-thread rendering, per-Clip cache growth, or user settings in the first pass.
- Flashing, installation, publication, or upstream submission.

## Display contract

- Own exactly three OLED rows: `OLED_MAIN_TOPMOST_PIXEL` through `OLED_MAIN_TOPMOST_PIXEL + 2`. On the 48-pixel display these are y=5..7; existing Clip text begins below them.
- Map the 128 OLED columns from Clip time zero through the complete Clip loop length, using 64-bit intermediate arithmetic. The mapping remains fixed while the user scrolls or zooms the pad matrix.
- Draw the current visible pad-matrix range as a distinct clipped selection span on that whole-Clip domain. Scrolling moves it; zooming changes its width; a viewport covering the whole Clip spans the full ruler.
- Keep three readable lanes: progress on the top row, viewport selection on the middle row, and musical marks on the bottom row. Draw quarter marks one pixel wide and every bar two adjacent pixels wide, shifting an end-edge bar inward rather than clipping it to one pixel. Draw Clip caps and the playhead vertically across all three rows after the lanes. Stopped Clips keep the whole-Clip ruler, marks, and viewport selection but hide moving progress.
- During clocked linear Arrangement Audio recording, treat the growing live position as the effective Clip length rather than compressing against the `kMaxSequenceLength` sentinel, and retain the existing right-edge playhead meaning. During a tempoless first-loop recording, show an explicit indeterminate full-width progress/playhead state with no invented beat marks until a musical length exists.
- Prefer quarter-note marks when they remain at least four pixels apart. Otherwise use bars, doubling the bar stride until marks remain sparse. Keep the mark count fixed and bounded.
- Use `Clip::getLivePos()` so Reverse and Ping-Pong motion remain truthful. Map cloned-overdub position the same way as the existing Session playhead. Pin tempoless or arrangement linear Audio recording to the growing right edge, matching Audio Clip View.
- Redraw only the three-row strip when its projected state changes. Static changes redraw immediately; moving-playhead and growing-domain projection is capped at 20 Hz through the existing 15 ms graphics routine, including when another event causes a full OLED render between cadence checkpoints, with no new timer.
- Notifications and popups remain authoritative overlays. Suppress incremental updates outside the supported normal Clip and non-Arranger Automation UIs, during stem export and view transitions, and while the playhead has no valid visible position.
- Keyboard View shows the same whole-Clip progress and beat reference without implying that note, chord, drum, or Norns pad columns are a timeline viewport. Its existing OLED chord, last-note, layout, title, and popup information remains authoritative below or over the ruler.

## Completion Criteria

- [ ] Normal Instrument, Audio Clip, and non-Arranger Automation OLED views show one consistent three-pixel top-edge ruler without obscuring titles, values, icons, or existing popups.
- [ ] Every Keyboard View layout shows the same whole-Clip transport reference without presenting its playable pads as a false timeline selection.
- [x] Beat and bar marks align with the whole Clip timeline and do not rescale when the pad view scrolls or zooms.
- [x] The playhead traverses the OLED once per complete Clip loop, wraps only at the Clip boundary, and does not reset merely because it leaves the visible pad range.
- [x] The viewport selection moves and resizes with horizontal scroll and zoom while remaining clipped to the whole-Clip ruler.
- [x] Redraws reuse existing UI cadence and remain fixed-cost with no heap allocation, storage access, sample scan, audio-engine call, or new timer from OLED drawing.
- [x] Focused tests cover whole-Clip geometry, selection clipping, wrap, zoom, scroll, stopped state, ordinary playback/recording, clocked Arrangement growth, tempoless first-loop recording, right-edge bars, and full-render cadence reuse.
- [x] The configured host suite and Release firmware build pass after the Keyboard and Velocity Drums surface expansion.
- [x] `nl/patch-oled-02` contains the corrected feature and the top `DEVIATIONS.md` describes the expanded observable behavior; all four descendants rebase cleanly afterward.
- [ ] A physical Deluge confirms legibility, non-interference, and smooth-enough motion before any flashing decision is called complete.

## Blockers

- None for source investigation and local implementation. Physical OLED balance and motion require the user's Deluge.

## Artifacts

- **A1: Compact OLED Clip progress ruler.**
	- **Type:** Local firmware feature and focused tests.
	- **State:** Software-complete and locally validated, including the combined host suite and Release build; physical verification pending.
	- **Contents:** Shared ruler geometry, supported Clip-view integration, redraw wiring, tests, and deviation contract.
	- **Location:** Local Community Firmware checkout.
	- **SCM surface:** Jujutsu change `pkolkoympqyoqzsntmuwqknrxzoyvslk`, bookmark `nl/patch-oled-02`, current commit `70038090f770`; four descendants rebased cleanly and the newest feature revision is `e97d0f036d32`.

## Decisions

- **Decision:** Keep this in the existing `oled` patch group.
	- **Rationale:** It is another OLED rendering feature; a new feature slug would fragment one source component.
- **Decision:** Treat the complete Clip loop as the ruler's horizontal domain and the visible 16-pad range as a separate selection.
	- **Rationale:** The ruler exists to provide whole-Clip orientation that the pad matrix cannot. Duplicating the visible pad range makes the playhead reset when it leaves the screen and removes the overview the user asked for.
- **Decision:** Give progress, viewport selection, and beat marks separate one-pixel lanes, then draw caps and playhead across them.
	- **Rationale:** A continuous selection line beside two-row bar teeth makes selection and meter visually ambiguous on a monochrome three-row display.
- **Decision:** Do not treat Arrangement recording's maximum-length sentinel or a tempoless provisional length as a finished Clip loop.
	- **Rationale:** The sentinel would collapse meaningful growth into one pixel, while tempoless recording has no truthful beat grid yet.
- **Decision:** Reuse existing playhead/redraw cadence.
	- **Rationale:** The indicator must not add an independent timer or audio-path workload.
- **Decision:** Cap moving-playhead OLED sends at 20 Hz while retaining one-pixel geometry.
	- **Rationale:** Each dirty OLED frame transfers the full 768-byte image over SPI shared with CV. A time cap bounds traffic without reducing spatial precision to four-pixel jumps.
- **Decision:** A full OLED redraw between cadence checkpoints repaints the cached ruler frame instead of sampling a new moving or growing projection.
	- **Rationale:** Replacing the canvas must restore the ruler, but an unrelated title or value redraw must not bypass the same 20 Hz work ceiling used by the partial-update path.
- **Decision:** Keep ruler state and geometry in `ClipView`, then opt supported Clip-editing surfaces into the same renderer and cadence rather than duplicating it.
	- **Rationale:** Automation edits such as velocity still belong to the current Clip and benefit from the same whole-loop orientation. Each additional surface must prove that the ruler remains truthful and that the three top rows are available.
- **Decision:** Add every non-Arranger Automation mode and keep Keyboard/Chord layouts excluded.
	- **Rationale:** Automation Overview, parameter editing, and Velocity all share the current Clip's timeline, scroll, and zoom, and leave the three top OLED rows free. Keyboard and Chord pads represent pitches, chords, or layouts rather than the Clip viewport, so the selection lane would be misleading.
- **Decision:** Reopen the former Keyboard/Chord exclusion and include the complete `KeyboardScreen` surface.
	- **Rationale:** The operator wants the whole-Clip transport reference to remain visible while performing. All seven playable layouts share one OLED owner, and none of their columns represent Clip time, so the ruler shows progress and musical marks without a viewport-selection span. Ordinary Instrument Clip View already covers the step-sequencer pad matrix; Song-level Performance View has no single authoritative Clip and remains excluded.
- **Decision:** Make the active display surface and viewport policy explicit in the shared ruler cache.
	- **Rationale:** Keyboard View reuses the current Instrument Clip but not its timeline viewport. Keying only on the underlying `InstrumentClipView` could retain a stale selection when switching surfaces. The Keyboard bridge must request an empty viewport and periodic refresh only while `KeyboardScreen` is the active UI.
- **Decision:** Repaint the ruler from Automation's full-canvas renderer and refresh it from Automation's existing graphics callback.
	- **Rationale:** Automation value updates replace the OLED canvas. Calling the shared renderer only from an outer Clip view would let those updates erase the ruler; reusing the shared cache preserves its 20 Hz projection ceiling without another timer or cache.
- **Decision:** Treat replacement of the OLED main canvas as ruler-cache invalidation, and never request a hidden OLED transfer while a top notification or the screensaver owns the display.
	- **Rationale:** Normal output and preset navigation can redraw the title without passing through the full Clip-view renderer. Geometry equality alone cannot prove that the cached ruler is still present. Hidden underlay maintenance may keep popup removal current, but it must not mark an unchanged visible frame dirty; the screensaver needs no ruler work at all.
- **Decision:** Keep static geometry in the shared cache and pass only compact moving/refresh state through the 15 ms callback.
	- **Rationale:** The first implementation returned and copied an 82-byte update and retained a second static-geometry copy. Release-symbol inspection measured a 1,732-byte preparation routine with 188 bytes of stack, so a revision comparison is both simpler and safer for a frequently called firmware UI path.
- **Decision:** Run broad firmware rebuilds once in a durable worker, redirect output, record timestamps, and leave build inputs unchanged until completion.
	- **Rationale:** A header rebuild took 5m39s on this machine. Treating that normal duration as a one-turn action and accepting a trivial header cleanup after a green build caused avoidable orchestration and a second broad compile.

## Workers

- `/root/keyboard_oled_path`: completed the read-only audit of Keyboard View OLED ownership, layouts, composition order, and the safest shared ruler hook.
- `/root/keyboard_ruler_tests`: completed the Pad View terminology, non-timeline viewport semantics, remaining-exclusion, and focused-coverage audit without editing files.
- `/root/keyboard_ruler_impl`: completed the bounded `KeyboardScreen` full-render and 20 Hz refresh bridges, explicit empty-viewport cache policy, formatting, and 24-case focused regression pass.
- `/root/keyboard_ruler_review`: completed adversarial review of UI ownership, cache transitions, popup/menu suppression, fixed work, test strength, and unintended behavior; no production defect remained after the policy-test correction.
- `/root/oled_ruler_tests`: completed the corrected whole-Clip domain, viewport-selection geometry, cadence and edge repairs, 23 focused cases, and `DEVIATIONS.md` update inside the existing `nl/patch-oled-02` revision.
- `/root/oled_ruler_layout`: completed adversarial review of view leakage, top-strip ownership, OLED composition, and cache replacement.
- `/root/oled_ruler_timing`: completed adversarial review of timeline projection, Triplet gaps, direction and recording semantics, frame pacing, and fixed work.
- `/root/oled_ruler_simplicity`: completed two adversarial passes; found the recording-domain, lane, full-render cadence, and right-edge bar defects, then passed the repaired revision without a remaining blocker.
- `/root`: owns plan, guidance, `DEVIATIONS.md`, Jujutsu integration, final checks, and user communication.

## Implementation Steps

- [x] Resolve Keyboard View and Pad View OLED ownership and record the supported surfaces and non-timeline ruler semantics.
- [x] Integrate the shared ruler after each supported surface's full-canvas draw without disturbing chord, last-note, layout, title, or popup content.
- [x] Extend focused coverage, run adversarial review, fold the source and test changes into `nl/patch-oled-02`, update the top `DEVIATIONS.md` contract, and rebuild the current stack once.
- [x] Audit OLED top-edge ownership, Clip-view render paths, playhead timing, viewport math, and focused test seams.
- [x] Fix the three-pixel non-clashing geometry, normal-Clip surface boundary, bounded marks, and 20 Hz partial-refresh contract.
- [x] Create fresh Jujutsu change `pkolkoympqyoqzsntmuwqknrxzoyvslk` above `nl/patch-oled-01`; bookmark it `nl/patch-oled-02` only after validation.
- [x] Implement the shared geometry and Clip-view integration with 14 focused behavior cases.
- [x] Review the complete diff, repair defects, run host checks and a Release build, then update `DEVIATIONS.md`.
- [x] Record the owner feedback loop, create `nl/patch-oled-02`, and leave physical OLED verification pending.
- [x] Replace viewport-domain geometry with whole-Clip geometry and a distinct viewport-selection span in the existing `nl/patch-oled-02` revision.
- [x] Re-run adversarial review, focused and configured host checks, formatter, and one durable Release build after the corrected behavior is folded.

## Work Log

- [x] [[2026-08-25]] - Reopened P40 for Keyboard View and Pad View transport coverage.
	- **Evidence:** The accepted contract deliberately suppressed the ruler in Keyboard and Chord layouts. The operator now reports that omission and explicitly extends the request to Pad View.
	- **Effect:** Keyboard View and all pad-performance layouts are in scope. The whole-Clip transport and beat reference should remain visible while performing, but pitch or chord pad columns must not be represented as a timeline viewport. `/root/keyboard_oled_path` and `/root/keyboard_ruler_tests` are running bounded read-only audits before source changes.
	- **Next:** Resolve the shared OLED owner and exact Pad View terminology, then update owner and repository guidance before editing firmware source.
- [x] [[2026-08-25]] - Accepted the Keyboard and Pad surface audit.
	- **Evidence:** Firmware has no UI named Pad View. Ordinary step-sequencer pads are `InstrumentClipView` and already show the ruler. Isomorphic, In Key, Piano, Chord, Chord Library, Velocity Drums, and Norns all share `KeyboardScreen`; the separate Song-level `PerformanceView` has no authoritative single Clip.
	- **Effect:** The operator clarified that Pad View means Velocity Drums. Add the whole-Clip ruler once at `KeyboardScreen`, with an empty viewport selection, which covers Velocity Drums and every sibling layout. Keep Performance View and the other Song-level surfaces excluded.
	- **Next:** `/root/keyboard_ruler_impl` will implement the shared full-render and 20 Hz refresh hooks with an explicit empty Keyboard viewport.
- [x] [[2026-08-25]] - Completed the first Keyboard View implementation and focused check.
	- **Evidence:** The source uses an explicit Keyboard surface and no-viewport policy, renders after Keyboard's normal OLED content, refreshes through Keyboard's existing graphics callback, and leaves the existing pad recording tick routine intact. The pinned formatter passed and `oled_clip_progress_ruler_spec` passed 24 cases.
	- **Effect:** The implementation is ready for adversarial review. The direct Release-object attempt correctly stopped outside the Nix environment because `ccache` was unavailable; no duplicate build was launched.
	- **Next:** Repair any accepted review finding, then fold source and tests into `nl/patch-oled-02` and run the canonical Nix validation once.
- [x] [[2026-08-25]] - Accepted a focused-test coverage correction.
	- **Evidence:** The first new case proves that an empty viewport preserves whole-Clip geometry, but it does not mechanically exercise the production viewport policy or cache transition and cannot prove that Keyboard's render hooks exist.
	- **Effect:** Move the small viewport policy into the pure ruler helper, make production apply it when preparing static geometry, and test Timeline to None to Timeline explicitly. The adversarial source-review gate remains responsible for verifying the two Keyboard hooks; no source-text test or large fake UI harness will be added.
	- **Next:** Apply the small policy seam, rerun the 24-case focused spec, and finish the runtime-path review.
- [x] [[2026-08-25]] - Passed the corrected implementation through adversarial review.
	- **Evidence:** The pure policy test now covers Timeline to None to Timeline, all 24 focused cases pass, and independent source review found the Keyboard full-render and refresh hooks, multiple-inheritance UI identity, cache ownership, 20 Hz cadence, menu and popup suppression, count-in, playback, recording, and existing Automation delegation sound.
	- **Effect:** No software finding remains before Jujutsu folding. Physical legibility, motion, and coexistence with chord and latest-note popups remain device checks.
	- **Next:** Fold source and tests into `nl/patch-oled-02`, keep the self-contained contract in the newest feature revision, then run the configured host suite and one logged Release build.
- [x] [[2026-08-25]]@23:55 - Completed the Keyboard and Velocity Drums software gate inside the existing OLED revision.
	- **Evidence:** Jujutsu folded source and tests into `nl/patch-oled-02` at `70038090f770`, rebased four descendants without conflict, and kept the updated contract in newest feature revision `e97d0f036d32`. The focused ruler spec passed 24 cases, formatter passed, and the configured host suite passed 14/14. One uninterrupted 166-step Release build ran from 23:49:27 to 23:55:11 PDT and took 331.41 seconds; the linker took 1.975389 seconds.
	- **Effect:** Every `KeyboardScreen` layout, including Velocity Drums, now receives the existing whole-Clip ruler without a viewport-selection span. The current BIN is 1,717,704 bytes with SHA-256 `98cb3399ad0391a74f4cadc1da5d8f860eb16b7ee4332db9544945cf156ae0e4`; the ELF SHA-256 is `cf918ac77f203df494ca214a3d6a8bdd915109570a892e5b320b6eb970f403ed`. No Git command, flash, installation, publication, or upstream submission occurred.
	- **Next:** Check all seven Keyboard layouts, especially Velocity Drums and chord/latest-note popups, on a physical OLED Deluge before calling P40 device-verified.
- [x] [[2026-08-24]] - Reopened the supported-surface boundary after operator hardware feedback.
	- **Evidence:** The current implementation deliberately suppresses the ruler in Automation View, so velocity editing loses the whole-Clip transport reference even though it still edits and follows the current Clip.
	- **Effect:** P40 again owns active design and source work. The existing `nl/patch-oled-02` revision will be rewritten in place after an audit maps Automation, Keyboard, and other Clip-editing surfaces; no new patch group or cancellation revision will be created.
	- **Next:** Record exact supported surfaces and OLED ownership before changing firmware source.
- [x] [[2026-08-24]] - Accepted the complete Clip-editor surface audit.
	- **Evidence:** All non-Arranger Automation modes use the current Clip's navigation domain and leave OLED rows 5 through 7 free. Keyboard and Chord layouts use pitch/layout navigation, while Arranger Automation uses the Arrangement domain. Automation also clears the whole OLED during live value updates.
	- **Effect:** P40 will add Automation only, repaint from Automation's own full-canvas path, reuse its existing graphics callback, and make inactive delegated Clip-view refresh a no-op instead of invalidating the active shared cache. Keyboard, Chord, and Arranger Automation remain excluded.
	- **Next:** Finish and isolate P41, then rewrite `nl/patch-oled-02` with this accepted surface expansion before rebuilding.

- [x] [[2026-08-24]]@18:57 - Created the OLED Clip-ruler feature plan before source edits.
	- **Evidence:** The user requested a compact top-edge progress bar with beat marks synchronized to playback, recording, and horizontal scrolling.
	- **Effect:** The feature is isolated as P40 in the existing OLED component. No firmware source, Jujutsu revision, build, flash, or Git operation has occurred.
	- **Next:** Audit the shared rendering, timing, and test seams.
- [x] [[2026-08-24]]@19:00 - Created the empty P40 working change before source edits.
	- **Evidence:** Jujutsu reports `pkolkoympqyoqzsntmuwqknrxzoyvslk` as an empty child of `nl/patch-oled-01`.
	- **Effect:** P40 has an isolated local revision boundary. No bookmark, source edit, build, flash, or Git operation occurred.
	- **Next:** Complete the three read-only audits, then fix the implementation contract before editing.
- [x] [[2026-08-24]]@19:09 - Completed the layout, timing, and test audits and fixed the implementation contract.
	- **Evidence:** The top three OLED rows are unused by normal Clip text; `ClipView::getTickSquare()` and 17 existing pad boundaries provide authoritative viewport geometry; the 15 ms graphics routine provides scheduling; every dirty OLED frame transfers 768 bytes over SPI shared with CV.
	- **Effect:** P40 is limited to normal Instrument and Audio Clip views, uses a pure fixed-capacity geometry helper, updates only its strip, and caps moving frames at 20 Hz. No source edit, build, flash, or Git operation occurred during the audits.
	- **Next:** Implement the helper, view integration, tests, local guidance, and deviation contract in the isolated Jujutsu change.
- [x] [[2026-08-24]]@19:49 - Completed the first implementation and software gate.
	- **Evidence:** The focused ruler spec passed 14 of 14 cases, the configured CTest suite passed 11 of 11 targets, formatter checks passed, and the ARM Release firmware linked. The final image is text 1,644,492, data 67,828, and bss 363,816: +3,540 text, +0 data, and +256 bss over the recorded pre-P40 image.
	- **Effect:** Normal Instrument and Audio Clip views now share one fixed 264-byte UI cache and a fixed-capacity geometry helper. The first pass corrected progress through Triplet gaps, offscreen frame-rate bypass, trailing Triplet recording extent, and unnecessary per-view cache duplication. No flash, publication, bookmark, or Git operation occurred.
	- **Next:** Run independent layout and timing reviews, repair accepted findings, rerun the affected checks, then create `nl/patch-oled-02`.
- [x] [[2026-08-24]]@20:02 - Accepted the adversarial integration findings for repair.
	- **Evidence:** Direct normal-view title redraws clear `OLED::main` without invalidating the ruler cache; ordinary top notifications fully cover the strip while incremental playhead updates can still dirty and transfer unchanged visible frames; the full-render path can draw during a fade transition that incremental refresh already excludes. The screensaver leaves the underlying Clip graphics callback active, and the first tests exercise only stateless cadence math rather than the cache/dirty decision. The deviation inventory also still says seventeen entries.
	- **Effect:** `/root/oled_ruler_tests` is assigned one focused repair batch. The repair must make canvas replacement observable to the cache, avoid hidden notification/screensaver transfer requests, apply the transition rule consistently to full and partial renders, expose and test the real refresh decision, and update the self-contained deviation inventory.
	- **Next:** Implement the smallest coherent repair, rerun the focused and full software gates, and submit the complete diff to a final adversarial review before creating `nl/patch-oled-02`.
- [x] [[2026-08-24]]@20:19 - Accepted a hot-callback stack and copy reduction before the bookmark.
	- **Evidence:** Release ELF inspection measured `prepareClipProgressRuler()` at 1,732 text bytes with 188 bytes of stack; `refreshClipProgressRuler()` adds 100 bytes. The 82-byte return value and duplicate displayed static geometry are copied and compared from the existing graphics callback. Live review of the refactor also found that an unchanged due projection must advance its cadence checkpoint or it will be recomputed every 15 ms forever.
	- **Effect:** The active repair must retain one cached static geometry, identify its displayed version by a small revision, and return only compact moving/refresh state. The OLED canvas-generation fix must use one OLED-wide counter rather than adding aligned state to every canvas. The 20 Hz projection gate applies even when the resulting visible frame is unchanged or offscreen.
	- **Next:** Measure the repaired symbols, cache size, BSS, and total image again as part of the post-review Release gate.
- [x] [[2026-08-24]]@20:27 - Completed the repaired software gate.
	- **Evidence:** Focused ruler cases passed 18/18 in 0.4s; configured CTest passed 11/11 in 5.06s test time and 8.5s including Nix startup; formatter passed; the uninterrupted ARM Release rebuild ran from 20:21:38 to 20:27:17, 339s, and linked in 2.055256s.
	- **Effect:** The final image is text 1,644,708, data 67,828, and bss 363,784: +3,756 text, +0 data, and +224 bss over the pre-P40 baseline. The shared cache is 196 bytes and the OLED generation is 4 bytes. The cheap pre-rate-limit path peaks near 112 stack bytes; due projection peaks near 280 bytes. Jujutsu change `pkolkoympqyoqzsntmuwqknrxzoyvslk` is at commit `6b6b2260b63ecb87fe63cfc4868f851576c1ef06`.
	- **Next:** Record the owner-local tool and guidance lessons before creating the bookmark; do not rebuild or alter firmware source again.
- [x] [[2026-08-24]]@20:30 - Reinstalled the Tasker_Plunger owner feedback loop after user correction.
	- **Evidence:** The owner thread matches this task, but `_owner/memory/`, `_owner/wart.tools.md`, and `_owner/wart.guidance.md` do not exist, and owner `AGENTS.md` does not instruct workers to read or maintain them. The Tasker_Plunger skill explicitly requires those actions when repeated friction and durable findings exist.
	- **Effect:** Added M1 owner runtime memory, W1 tool warts, W2 guidance warts, owner entry-point links, standing feedback-loop rules, and matching checkout-local build guidance. They record the exact sandbox, uv, QMD, and long-build responses without widening global guidance.
	- **Next:** Run the plan doctor, then create `nl/patch-oled-02` without another firmware build.
- [x] [[2026-08-24]]@20:32 - Completed the software handoff without another source edit or build.
	- **Evidence:** Tasker_Plunger doctor checked 4 groups and 40 plans with 0 errors and 0 warnings. Jujutsu bookmark `nl/patch-oled-02` points to change `pkolkoympqyoqzsntmuwqknrxzoyvslk`, commit `6b6b2260b63ecb87fe63cfc4868f851576c1ef06`.
	- **Effect:** P40 is software-complete and remains active only for physical OLED legibility, notification return, and simultaneous CV/audio verification. No Git command, flash, publication, or additional build occurred.
	- **Next:** Test the bookmarked firmware on a physical OLED Deluge before making any flashing or visual-completion claim.
- [x] [[2026-08-24]]@20:37 - Reopened the ruler geometry after operator correction.
	- **Evidence:** The existing implementation maps the visible 16-pad viewport to the full OLED, so scrolling or playback beyond that viewport rescales or resets the display. The operator clarified that the OLED must show the whole Clip plus the current visible selection, not duplicate the pad matrix.
	- **Effect:** The prior software gate is retained as historical evidence for the old implementation but no longer proves the requested behavior. Ccache work is paused; P40 again owns the active firmware change.
	- **Next:** Audit the current geometry and replace it with a fixed whole-Clip domain plus a distinct scroll/zoom selection span before any new build.
- [x] [[2026-08-24]]@20:43 - Accepted the first whole-Clip adversarial findings before validation.
	- **Evidence:** Linear Arrangement recording uses `kMaxSequenceLength` as a provisional loop length, and tempoless first-loop recording lacks a settled musical length. A continuous selection row would also visually merge with two-row bar marks.
	- **Effect:** Clocked Arrangement growth uses the live extent, tempoless recording is explicitly indeterminate without beat marks, and the three OLED rows have separate progress, selection, and mark roles.
	- **Next:** Add focused cases for both recording states and confirm the final geometry remains fixed-cost.
- [x] [[2026-08-24]]@20:59 - Accepted two post-fix review findings before final validation.
	- **Evidence:** The full-render path still re-projects moving and growing state whenever unrelated OLED content redraws, bypassing the intended 20 Hz ceiling. A bar projected at column 127 is also clipped to one pixel despite the two-pixel contract.
	- **Effect:** Full renders must reuse the cached moving frame between cadence checkpoints unless a structural, transport, or finalized-length change is urgent. Right-edge bars shift inward to remain two pixels wide.
	- **Next:** Repair both cases, add focused pixel/cadence tests, then repeat host and Release gates.
- [x] [[2026-08-24]]@21:05 - Completed the corrected whole-Clip software gate.
	- **Evidence:** Focused ruler cases passed 23/23 in 0.34s; configured CTest passed 11/11 in 5.02s; formatter passed; post-fix review passed; and the logged Release build ran from 21:04:53 to 21:05:48, with Ninja reporting 44.102s and the linker 2.046352s. The final BIN SHA-256 is `b3a4ad612a928bf2b62cbbfed5f4c7cbea11ac41aa22cb438ade54dc632ee204`; ELF SHA-256 is `044379262f46c5e693933857db10248436d3d84d35bb3aa86f8be494d58973f4`.
	- **Effect:** `nl/patch-oled-02` now uses a fixed whole-Clip domain, separate viewport selection, truthful growing and tempoless recording states, disjoint three-row lanes, cadence-safe full redraws, and two-pixel edge bars. The final image is text 1,644,212, data 67,828, and bss 363,720: +3,260 text, +0 data, and +160 bss over the pre-P40 baseline, and 560 total bytes smaller than the rejected viewport-domain build. The shared cache is 140 bytes; the OLED generation remains 4 bytes. Release disassembly measures 176 bytes of own frame for preparation, 72 for full rendering, and 96 for incremental refresh; the direct refresh-to-preparation nesting is 272 bytes.
	- **Next:** Validate legibility, selection meaning, notification return, motion, and simultaneous CV/audio behavior on the physical Deluge before calling P40 device-verified.
- [x] [[2026-08-24]]@21:38 - Revalidated the ruler after P30's parent revision gained the 128-column waveform default.
	- **Evidence:** Jujutsu rebased the unchanged P40 change to current commit `1d7c88b1d2c276c87c2288c23a826d4aa71a667c` without conflicts. The combined configured CTest suite passed 11 of 11, including `oled_clip_progress_ruler_spec`, and the logged Release image linked successfully.
	- **Effect:** The ruler remains the direct child `nl/patch-oled-02`; its whole-Clip and separate-viewport contract is unchanged. The combined image size and hashes now belong to P30's later validation record rather than replacing P40's historical isolated size comparison.
- [x] [[2026-08-25]]@00:09 - Completed the non-Arranger Automation expansion inside the existing OLED revision.
	- **Evidence:** Focused ruler cases passed 23/23, affected ARM objects and formatting passed, and adversarial runtime review found no remaining issue. P41 rebased cleanly above P40. The combined configured CTest suite passed 11/11, and the single logged Release build linked in 56.36 seconds with BIN SHA-256 `484aafabd9c59952e176890525e4853af5dabf2daf687ee87a41a94ddddc5aae` and ELF SHA-256 `d63ce5405b4b1b3890785f29e13b2cd5c91b2d37e6658a717a57eddc10647ec3`.
	- **Effect:** `nl/patch-oled-02` is now at commit `1230870ec46b` and draws the ruler in normal Instrument and Audio Clip views plus all non-Arranger Clip Automation views. Keyboard, Chord, and Arranger Automation remain excluded. Only physical OLED validation remains.

## Validation

- **Command or action:** Focused `oled_clip_progress_ruler_spec`, configured CTest suite, formatter, Release build, size comparison, complete Jujutsu diff review, and physical OLED/CV checklist.
	- **Expected result:** Correct whole-Clip progress and viewport-selection mapping with bounded redraw cost and no obstruction of existing OLED information.
	- **Observed result:** The current Keyboard expansion passes the corrected 24-case focused spec, pinned formatter, independent runtime-path review, and all 14 configured host tests. Jujutsu folded it into `nl/patch-oled-02` at `70038090f770` and rebased four descendants cleanly. The single logged 166-step ARM Release build took 331.41 seconds and linked successfully. The 1,717,704-byte BIN SHA-256 is `98cb3399ad0391a74f4cadc1da5d8f860eb16b7ee4332db9544945cf156ae0e4`; the ELF SHA-256 is `cf918ac77f203df494ca214a3d6a8bdd915109570a892e5b320b6eb970f403ed`.
	- **Failure signals:** Off-beat marks after scroll or zoom, wrap jitter, stale recording state, title/icon overlap, a new timer, work from OLED drawing that touches audio/storage/sample data, or unrelated source changes.
	- **Interpretation:** Software checks can establish geometry, integration, and buildability; only hardware can establish final visual balance.

## Unfinished Work

- [x] Run the configured host suite and one logged Release build after folding Keyboard View and rebasing all four descendants.
- [ ] Run the physical OLED check before calling the feature device-verified.
