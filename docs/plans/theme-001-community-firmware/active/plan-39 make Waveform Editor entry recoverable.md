---
description: "Make the Waveform Editor an explicit graphical child of sample information with clear Start and End control."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Make Waveform Editor entry recoverable

## Purpose

Keep whole-sample editing distinct from Audio Clip View. Opening a sample-bound control should show sample information first, Select should enter the graphical Waveform Editor, and Back should return to the information screen instead of skipping it. The graph should show both sample bounds and let Shift + Select choose which bound Select rotation controls. This plan began on [[2026-08-25]].

## Current State

- Status: software-complete; the Waveform Editor behavior is implemented in Jujutsu change `tpnnwp` at bookmark `nl/patch-sample-editor-02`, current rebased commit `919731bfd645`. Focused ARM objects, formatting, all 14 configured host tests, adversarial source review, the self-contained deviation contract, and the current combined Release build pass.
- Next action: leave physical entry, return, bound-selection, and Kit-audition validation to the operator before calling the feature device-verified.

## Task boundary

- This plan owns the Waveform/Sample Marker Editor interaction.
- P38 [[recoverable Audio Clip start editing]] separately owns source-backed pre-start navigation and recoverable Start movement in Audio Clip View.
- Neither task replaces, merges, or impersonates the other editor.

## Product integration

- **Existing model:** Sample Start and End shortcuts can launch the graphical Sample Marker Editor directly. Some launchers deliberately bypass their parent sample menu, and closing the graph can skip the sample-information/settings surface. The graph already knows every active marker and can draw selected and unselected bounds, but it has no Shift + Select Start/End switch.
- **User intent:** The graphical editor should be an explicit mode, not the default landing screen. The player should see where they are, enter deliberately, return predictably, and switch the controlled bound without hunting for another shortcut.
- **Integrated model:** Every supported Waveform Editor launcher lands on its existing sample-information/settings screen with WAVEFORM or the relevant bound item focused. Plain Select enters the graph. Back closes the graph and restores that same parent screen. The graph renders Start and End together and uses Shift + Select to switch the selected sample bound.
- **Architecture impact:** Keep SoundEditor as the menu-stack owner and SampleMarkerEditor as the graphical owner. Consolidate duplicate launch adapters where they represent one Waveform action. Do not change Audio Clip View timeline controls, sample playback arithmetic, stored project data, or create another renderer.

## Scope

### In scope

- Stop supported Waveform Editor shortcuts and menu items from opening the graph immediately.
- Land on the existing sample-information/settings screen with the graphical action focused.
- Enter the graph only from a plain Select press.
- Return from the graph to the same information screen with Back; the next Back follows normal SoundEditor navigation.
- Draw sample Start and End together whenever they lie in the current graphical viewport. Keep the controlled bound visually dominant and the other visible but dim.
- Switch the controlled playback-relative Start or End with one latched Shift + Select press and consume its matching release.
- Preserve plain Horizontal scroll, pressed-Horizontal zoom, Select rotation of the controlled marker, marker-pad selection, loop-marker behavior, OLED rendering, and 7SEG position display unless feedback must identify the newly selected bound.
- Preserve the current controlled bound across a graph-to-information-to-graph round trip for the same sample, and reset predictably when the edited sample changes.
- Add focused host or compile checks, adversarial review, a logged Release build, and a physical-device checklist.

### Out of scope

- Audio Clip View pre-start space, Clip-time marker selection, `loopLength` changes, time-stretch behavior, or Plan 38 arithmetic and history.
- Changing raw sample-bound semantics, loop-point creation/removal, waveform resolution, OLED waveform caching, sample audition behavior, Kit row selection, Slicer, recording, or the WAV file.
- Forcing both bounds to remain onscreen after the user zooms or scrolls them out of the viewport; persistent edge indicators or an always-full overview require separate direction.
- Flashing, installation, publication, or an upstream pull request.

## Behavior contract

- A Waveform Editor shortcut opens the existing sample-information/settings surface first. It does not draw the graph during entry.
- Plain Select on the focused Waveform action enters the graph. Shift + Select on the information screen does not enter it.
- Back from the graph returns to the exact parent information screen and focused item. Back from that screen retains normal menu behavior.
- On entry, sample Start is the default controlled bound unless the same sample is being resumed after Back; resuming retains the bound selected before exit.
- The graph draws Start and End at their raw sample positions whenever those positions are inside the visible source viewport. Selected and unselected bounds remain distinguishable on pads and OLED.
- Shift + Select switches Start to End or End to Start once on press. The graph consumes both edges even if Shift is released before Select.
- The selected-bound label and color remain playback-relative when the sample is reversed. The switch never selects a loop marker.
- Select rotation moves only the selected marker through the graph's existing movement rules. Horizontal scroll and pressed-Horizontal zoom remain unchanged.
- Kit Waveform Editor bare-Select audition remains momentary and unchanged. Shift + Select switching must not start, stop, or latch that audition.
- Card-routine deferral, invalid/no-sample refusal, and existing UI-mode guards remain intact. A rejected entry or switch does not leave stale selection or consume a later unrelated Select press.

## Completion Criteria

- [x] Supported sample-bound shortcuts land on sample information instead of opening the graph.
- [x] Plain Select enters the graph; Back returns to the same information screen; the next Back navigates normally.
- [x] Start and End render together in the graph when in view, with distinct controlled-bound feedback in both display paths.
- [x] Shift + Select switches Start and End exactly once per press and consumes the matching release.
- [x] The selected bound survives one Back/re-entry cycle for the same sample and resets for a different sample.
- [x] Horizontal navigation, marker movement, loop markers, reverse labels/colors, and Kit bare-Select audition retain their established behavior.
- [x] Audio Clip View and Plan 38 remain in a separate revision and interaction path.
- [x] Focused checks, affected ARM compilation, formatting, adversarial review, all 14 configured host tests, and one uninterrupted logged Release build pass.
- [ ] A physical Deluge confirms entry, return, selection feedback, modifier ownership, and Kit audition compatibility before device verification is claimed.

## Artifacts

- **A1: Waveform Editor entry and bound-selection contract.**
	- **Type:** Active implementation plan.
	- **State:** Software-complete; physical-device validation remains.
	- **Location:** This plan.
	- **SCM surface:** Jujutsu change `tpnnwp`, current rebased commit `919731bfd645`, bookmark `nl/patch-sample-editor-02`, above Plan 38's Audio Clip revision.
- **A2: Waveform Editor navigation and bound-selection implementation.**
	- **Type:** Firmware source revision.
	- **State:** Focused ARM objects, formatting, host tests, adversarial review, and the combined Release build pass; no device test yet.
	- **Contents:** Routes supported launchers through their sample-information parent, opens the graph with plain Select, returns with Back, draws both visible bounds, retains same-sample selection, switches playback-relative Start and End with latched Shift + Select, and protects Kit audition and modifier release order.
	- **Location:** Local Community Firmware checkout under `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware`.

## Source entry points

- **S1:** `src/deluge/gui/menu_item/audio_clip/sample_marker_editor.cpp` adapts Audio Clip Sample/WAVEFORM entry to the global graph.
- **S2:** `src/deluge/gui/menu_item/sample/loop_point.cpp` adapts sampled Synth and Kit Start/End items to the same graph.
- **S3:** `src/deluge/gui/ui/sound_editor.cpp` owns shortcut-to-parent-menu routing and focus restoration after the graph closes.
- **S4:** `src/deluge/gui/ui/sample_marker_editor.cpp` owns marker selection, dual-bound rendering, Select movement, Shift + Select handling, Back, OLED/7SEG feedback, and Kit audition.
- **S5:** `src/deluge/gui/ui/menus.cpp` owns the parent sample menus and Waveform launcher objects.

## Decisions

- **Decision:** Keep this feature in the `sample-editor` patch group.
	- **Rationale:** It changes the Waveform/Sample Marker Editor interaction and follows the existing Kit-row audition revision in that component.
- **Decision:** Preserve the two-editor product model.
	- **Rationale:** Audio Clip View edits Clip time and playback boundaries. Waveform Editor inspects the source sample. Their navigation, controls, and behavior contracts remain separate.
- **Decision:** Use the existing parent menu as sample information.
	- **Rationale:** It already contains file and sample settings and preserves normal SoundEditor navigation. A second information UI would duplicate the same context.
- **Decision:** Shift + Select switches sample Start and End only.
	- **Rationale:** It gives one coherent two-bound control while leaving loop-marker pad interactions and Kit bare-Select audition intact.

## Implementation Steps

- [x] Record the operator's exact interaction and the two-editor boundary.
- [x] Create `nl/patch-sample-editor-02` and move only the existing Waveform navigation files into it with Jujutsu.
- [x] Reconcile Audio Clip and sampled Synth/Kit launchers so each uses its parent sample-information screen before the graph.
- [x] Add same-sample selection lifecycle and latched Shift + Select Start/End switching.
- [x] Verify the existing renderer draws both bounds and add only missing selected/unselected or 7SEG feedback.
- [x] Preserve Kit bare-Select audition and all existing navigation, loop-marker, reverse, card, and no-sample behavior.
- [x] Add the self-contained behavior to the top revision's `DEVIATIONS.md` without source code or implementation recipes.
- [x] Run focused tests, formatting, adversarial review, affected ARM compilation, configured host tests, and one uninterrupted logged Release build.
- [ ] Leave flashing and physical-device validation to the operator.

## Work Log

- [x] [[2026-08-25]]@01:32 - Split the Waveform Editor fixup from Plan 38 after the operator clarified the two tasks.
	- **Evidence:** The operator explicitly distinguished Audio Clip View from the Waveform/Sample Editor and repeated the five Waveform Editor requirements separately from applying Plan 38.
	- **Effect:** This plan and `nl/patch-sample-editor-02` own the Waveform interaction. P38 and `nl/patch-audio-clip-01` retain Audio Clip View recovery.
	- **Next:** Separate the current shared working change by component before further source edits.
- [x] [[2026-08-25]]@02:21 - Completed the separate Waveform Editor implementation and focused software checks.
	- **Evidence:** Jujutsu change `tpnnwp` contains only the nine Waveform Editor navigation, menu, and UI files. The five affected ARM objects compiled, clang-format passed, all 12 configured host tests passed, and adversarial review found and then verified fixes for mixed pointer types and Shift-release-first behavior on vertical/7SEG menus.
	- **Effect:** Supported shortcuts now land on sample information; plain Select enters the graph; Back returns; both visible bounds render; same-sample bound selection persists; and Shift + Select switches Start/End without leaking into Kit audition. Audio Clip View source is not part of this revision.
	- **Next:** Keep this change untouched while Plan 38 is integrated below it, then add the combined deviation contract and run one Release build.
- [x] [[2026-08-25]]@02:09 - Passed the combined Plan 38 and Plan 39 Release gate without restarting the build.
	- **Evidence:** The single Nix Release process ran from 02:08:18 through 02:09:50 PDT and exited zero after 228 steps. It produced `build/Release/deluge.bin`, 1,718,640 bytes, SHA-256 `808d2bdf8f4941bde4b826aef3e0895a1fc5d88d5c983cd3790661b4d9411af7`. The final code-only format check and all 13 configured host tests pass. The redirected log is `/private/tmp/deluge-p38-p39-release.log`.
	- **Effect:** Every local software gate in this plan passes. No firmware was flashed, installed, published, or submitted, and no Git command was used.
	- **Next:** Physically verify every launcher, Select entry, Back return, Shift + Select in both release orders, same-sample re-entry, reverse labels, loop markers, and Kit held-Select audition.
- [x] [[2026-08-25]]@02:22 - Revalidated the Waveform Editor fixup as a separate revision after the operator restated the two-task boundary.
	- **Evidence:** Jujutsu still shows `nl/patch-sample-editor-02` above, not folded into, Plan 38. Its source diff is limited to Waveform Editor launchers, sample-information navigation, graph controls and rendering state, plus the top-level deviation contracts. The refreshed formatter check and all 13 configured host tests pass. The existing Release BIN still has the recorded size and SHA-256.
	- **Effect:** No source or revision change was needed. The information screen, deliberate Select entry, Back return, dual-bound display, and Shift + Select switching remain independent from Audio Clip View.
	- **Next:** Leave physical entry, return, bound-selection, and Kit-audition validation to the operator.

## Validation

- **Command or action:** Navigation and control checks.
	- **Expected result:** Information → Select → graph → Back → information works for each supported launcher; Shift + Select switches one bound and never leaks to Kit audition or lower UI layers.
	- **Observed result:** Source tracing and adversarial review cover Audio Clip, sampled Synth/Kit, multi-range, reverse, Kit audition, card deferral, and modifier release order. The five affected ARM objects compile. Physical behavior is not device-verified.
	- **Failure signals:** Immediate graph entry, Back skipping information, stale focus, double toggle, leaked release, or stopped/latched Kit audition.
	- **Interpretation:** Passing proves deterministic UI ownership; hardware still must confirm control feel.
- **Command or action:** Rendering and compatibility checks.
	- **Expected result:** Start and End both render when in view, the selected one is unmistakable, Horizontal and Select movement retain their roles, and the separate Audio Clip View is unchanged.
	- **Observed result:** Existing pad and OLED render paths enumerate Start through End, the selected marker remains dominant, clang-format passes, all 14 configured host tests pass, and the latest full-stack Release build succeeds. Physical checks remain pending.
	- **Failure signals:** Missing unselected bound, wrong reverse label/color, loop-marker selection, viewport change, or Plan 38 behavior change.
	- **Interpretation:** Passing proves the graph still represents the existing sample domain rather than becoming a second Audio Clip editor.

## Unfinished Work

- [x] Separate, complete, and locally validate the Waveform Editor revision short of the combined Release build.
- [ ] Validate entry, Back, Shift + Select, and Kit audition on a physical Deluge before any flashing decision.

### Physical check

- [ ] Open WAVEFORM or a supported sample-bound shortcut from an Audio Clip, sampled Synth, and Kit. Each must land on sample information, not the graph.
- [ ] Press Select to enter the graph, then Back. Back must return to the same information item; the next Back must navigate normally.
- [ ] Zoom so Start and End are both visible. Both must render, with the controlled bound clearly dominant.
- [ ] Press Shift + Select once in each modifier-release order. Each press must switch Start or End exactly once without a leaked second action.
- [ ] In a Kit, bare Select must still audition momentarily and release cleanly; Shift + Select must switch bounds without auditioning. Horizontal and pressed Horizontal must remain scroll and zoom.

Related: P38 [[recoverable Audio Clip start editing]] and P41 [[audition current sample in Waveform Editor]].
