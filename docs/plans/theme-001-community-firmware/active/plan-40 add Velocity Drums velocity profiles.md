---
description: "Decouple Velocity Drums pad size from velocity resolution with Full, four-quadrant, two-half, and fixed profiles."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Velocity Drums velocity profiles

## Purpose

Keep the established large Velocity Drums pads while letting the player choose fewer, larger velocity targets. This plan began on [[2026-08-26]].

## Current State

- Status: software-complete; physical validation remains.
- Result: `nl/patch-kb-drums-01`, Jujutsu change `ulztwqmrxzvzxynuxpvxmmnxqzkpkrtz`, commit `7eeb8f6667f419762976315559d29063f42e181a`, adds the four saved profiles, Scale controls, matching new-profile brightness, compatibility loading, focused tests, and deviation 22. Formatting, adversarial review, all 15 configured host tests, and the one 212.88-second ARM Release build pass.
- Next action: physically check control feel, quadrant orientation, brightness, held notes, Load and layout-change collisions, and save/reload before claiming device verification.

## Task boundary

- This plan owns only Velocity Drums velocity profiles and their controls, rendering, persistence, and tests.
- The separate proposed double-press Horizontal note-length gesture is not part of this plan.
- The pre-existing Zoom 3 widened-block velocity mismatch and Zoom 13 zero-velocity cell remain separate bug work. This feature must not silently repair or spread them.

## Product integration

- **Existing model:** Velocity Drums couples each drum block's physical cell count to its velocity resolution. A 4 by 4 drum therefore has sixteen small velocity targets even when the player chose that zoom for a large hit area. Horizontal rotation scrolls drums and pressed-Horizontal rotation changes block size. Scale has no useful Kit action.
- **User intent:** Preserve the same drum placement and large blocks while choosing four large velocity quadrants, two large velocity halves, or one fixed velocity.
- **Integrated model:** Keep one Velocity Drums layout and add a saved velocity profile. Scale becomes a momentary Kit-only modifier for choosing the profile and fixed value; it does not put the Kit into musical Scale mode.
- **Architecture impact:** Add two small fields to the existing per-Clip drums keyboard state, route the modifier only while Velocity Drums owns a Kit, and make one mapping function drive both sounded velocity and pad brightness. Do not create a duplicate layout or another timer.

## Scope

### In scope

- Add `FULL`, `4`, `2`, and `FIXED` profiles to Velocity Drums.
- Preserve `FULL` as the exact current mapping and as the default for old and new Clips.
- In the common 4 by 4 block, map `4` to four 2 by 2 quadrants: soft bottom-left, medium bottom-right, loud top-left, maximum top-right.
- Map `2` to bottom and top halves. Use left and right halves only when a block has one row.
- Map `FIXED` to one saved velocity from 1 through 127 across the complete drum block.
- For blocks that cannot form the requested regions, use the available dimensions to produce no more than the selected number of ordered regions; a one-cell block uses the saved fixed velocity.
- Hold Scale and turn Horizontal to select the profile. A Scale press with no encoder turn only shows the current profile.
- Hold Scale and turn Vertical to adjust the fixed velocity; editing it selects `FIXED`.
- Match pad brightness to the velocity that each cell will emit.
- Save the profile and fixed value with the existing per-Clip Velocity Drums state. Missing or invalid fields load as `FULL` with a safe fixed value.
- Add focused mapping and persistence checks, affected ARM compilation, formatting, configured host tests, adversarial review, a self-contained `DEVIATIONS.md` contract, and one logged Release build.

### Out of scope

- The note-length double-press gesture, melodic Keyboard layouts, ordinary Instrument Clip scale mode, Kit-row sample settings, sequencer note velocity editing, MIDI velocity processing, or a second Fixed Drums layout.
- Changing drum order, scroll, zoom, selected-drum tracking, retrigger behavior, recording, note-off behavior, or the existing `FULL` mapping.
- Repairing the two known pre-existing Velocity Drums edge defects, flashing, installation, publication, or an upstream pull request.

## Behavior contract

- Existing songs and Clips render and play exactly as before until the player chooses another profile.
- `FULL` preserves every current cell velocity and brightness.
- At 4 by 4, `4` maps the complete quadrants to velocities 32, 64, 96, and 127 from bottom-left through top-right. Every cell in one quadrant has identical brightness and emitted velocity.
- At 4 by 4, `2` maps the lower two rows to velocity 64 and the upper two rows to velocity 127.
- `FIXED` maps every cell to the saved fixed velocity and uniform brightness.
- Scale + Horizontal changes only the profile. Scale + Vertical changes the fixed value and selects `FIXED`. Plain Horizontal and pressed-Horizontal retain scroll and zoom.
- Scale alone shows the active profile and consumes its release without the current Kit `CANT` response. Other Scale shortcuts and non-Velocity-Drums surfaces retain their existing behavior.
- A profile or fixed-value change does not retrigger, stop, or rewrite an already sounding or recorded note. It applies to later pad presses.
- Profile and fixed-value loading reject invalid values without corrupting the remaining Keyboard state.

## Completion Criteria

- [ ] All four profiles emit the specified velocities and render matching brightness across every supported drum-block geometry.
- [x] The 4 by 4 `4` profile uses four 2 by 2 quadrants, not horizontal stripes.
- [x] Velocity Drums accepts the Scale modifier without changing musical Scale mode; other views retain existing Scale behavior by source and adversarial review.
- [ ] Profile and fixed value survive save and reload, while old projects default to exact `FULL` behavior.
- [x] Scroll, zoom, selection, retrigger, recording, and the existing `FULL` mapping remain unchanged by the reviewed software paths.
- [x] Focused tests, formatting, affected ARM compilation, configured host tests, adversarial review, and one logged Release build pass.
- [ ] A physical Deluge confirms control feel, quadrant placement, brightness, retriggering, and persistence before device verification is claimed.

## Artifacts

- **A1: Velocity Drums velocity-profile feature.**
	- **Type:** Local firmware feature and focused tests.
	- **State:** Software-complete; physical validation pending.
	- **Location:** Local Community Firmware checkout.
	- **SCM surface:** Jujutsu bookmark `nl/patch-kb-drums-01`, change `ulztwqmrxzvzxynuxpvxmmnxqzkpkrtz`, commit `7eeb8f6667f419762976315559d29063f42e181a`.
	- **Build artifact:** `build/Release/deluge.bin`, 1,719,540 bytes, SHA-256 `d41fad6421e01d9bde2e6a2857c7fd7f98af8a79436e339bd2f94250c1b3a9ea`.

## Source entry points

- **S1:** `src/deluge/gui/ui/keyboard/layout/velocity_drums.h` and `.cpp` own drum geometry, cell velocity, rendering, and Velocity Drums encoder behavior.
- **S2:** `src/deluge/gui/ui/keyboard/state_data.h` owns per-Clip drums keyboard state.
- **S3:** `src/deluge/model/clip/instrument_clip.cpp` saves and loads Keyboard state.
- **S4:** `src/deluge/gui/ui/keyboard/keyboard_screen.cpp` routes Scale and encoder input for the active layout.
- **S5:** `tests/spec/` owns focused pure behavior checks.

## Decisions

- **Decision:** Extend Velocity Drums instead of adding another layout.
	- **Rationale:** Drum placement, scroll, zoom, selection, and performance behavior remain one coherent surface; only velocity resolution changes.
- **Decision:** Use four quadrants for profile `4`.
	- **Rationale:** A 4 by 4 drum becomes four genuine 2 by 2 hit targets instead of four narrow strips.
- **Decision:** Reuse Scale only as a momentary Velocity Drums modifier.
	- **Rationale:** Kits cannot use musical Scale mode, while modifier plus encoder keeps profile changes deliberate and leaves no hidden mode active.
- **Decision:** Keep `FULL` bit-for-bit compatible.
	- **Rationale:** Existing songs and muscle memory must not change merely because the new fields exist.

## Workers

- `/root`: completed the plan, guidance, Jujutsu integration, final diff review, validation, and user communication; status: completed; next: physical-device follow-up only.
- `/root/keyboard_ruler_impl`: completed the bounded production implementation and accepted review repairs; status: completed; result: saved profile state, Scale routing, velocity mapping, brightness, and feedback compile in the ARM Release image.
- `/root/keyboard_ruler_tests`: completed the focused pure-behavior suite; status: completed; result: nine profile scenarios execute in the configured host suite.
- `/root/keyboard_ruler_review`: completed independent input-collision, compatibility, mapping, and simplicity review; status: completed; result: no P1 or P2 defect remains after stale-gesture, render-cache, input-precedence, persistence, and 7SEG feedback repairs.

## Implementation Steps

- [x] Record the separate feature boundary and accepted quadrant behavior before source edits.
- [x] Update owner and checkout guidance for the exact accepted behavior and revision group.
- [x] Create and name the isolated `nl/patch-kb-drums-01` Jujutsu change.
- [x] Implement profile state, save/load compatibility, Scale input routing, velocity mapping, and matching pad brightness.
- [x] Add focused mapping and persistence-field checks without changing the legacy `FULL` behavior.
- [x] Run adversarial review and repair every accepted defect without widening the feature.
- [x] Fold the self-contained `DEVIATIONS.md` contract into the newest feature revision.
- [x] Run formatting, affected ARM compilation, configured host tests, and one uninterrupted logged Release build.
- [x] Leave flashing and physical-device validation to the operator.

## Learning Log

- `FULL` currently uses a separate exponential brightness curve rather than deriving brightness linearly from its emitted velocity. Preserve both legacy outputs exactly; share one velocity-and-brightness policy only for the new profiles.
- A Velocity Drums Scale press needs explicit ownership through its matching release. Consume that release even if the active layout or Output changes while Scale remains held.
- Scale-modified encoders must take precedence over Shift colour editing and existing pressed-Horizontal or Shift zoom behavior. A Load chord must not display or mutate the velocity profile.
- Parse the saved profile as a raw value and validate it before casting. Validate fixed velocity independently so one invalid field cannot reset scroll, zoom, or the other new field.

## Work Log

- [x] [[2026-08-26]]@10:46 - Created the Velocity Drums feature plan after the operator approved implementation.
	- **Evidence:** The operator separated note-length editing from the large-pad velocity feature, accepted `FULL`, `4`, `2`, and `FIXED`, and specified four quadrants for profile `4`.
	- **Effect:** This plan owns only Velocity Drums velocity profiles. The note-length shortcut remains separate and unimplemented.
	- **Next:** Update standing guidance, create the isolated Jujutsu change, and dispatch the bounded implementation and test workers.
- [x] [[2026-08-26]]@10:52 - Accepted the pre-implementation input, persistence, and compatibility guardrails.
	- **Evidence:** Source review found Shift + Vertical colour precedence, layout-change release leakage, independent raw persistence validation, and the legacy Full brightness curve as concrete risks.
	- **Effect:** P44 will latch Scale ownership, preserve Full velocity and brightness exactly, prioritize the new modifier only in Velocity Drums, reject Load chords, and validate each saved field without disturbing existing drums state.
	- **Next:** Complete the first production and focused-test pass under these constraints, then review the integrated diff.
- [x] [[2026-08-26]]@11:02 - Integrated the production and focused-test passes and closed the adversarial review findings.
	- **Evidence:** Review found and the implementation repaired stale Scale ownership after leaving Keyboard View, repeated render-loop brightness work, indistinguishable 7SEG labels, input-precedence conflicts, and coupled invalid-field loading. The final reviewer found no remaining P1 or P2 defect.
	- **Effect:** `FULL` retains its separate legacy paths. The new profiles use bounded cached brightness mapping, Scale ownership ends safely, Load suppresses mutation and feedback, and each saved field validates independently.
	- **Next:** Add the self-contained deviation contract and run the final software gate without further source edits.
- [x] [[2026-08-26]]@11:11 - Passed the final software gate and named the local revision.
	- **Evidence:** `clang-format --dry-run --Werror` passed all eight changed C++ files; all 15 configured CTest targets passed; the ARM Release build completed 96 of 96 steps in 212.88 seconds. The build emitted only the existing `view.cpp` fallthrough and RWX LOAD-segment warnings. `build/Release/deluge.bin` is 1,719,540 bytes with SHA-256 `d41fad6421e01d9bde2e6a2857c7fd7f98af8a79436e339bd2f94250c1b3a9ea`.
	- **Effect:** Jujutsu bookmark `nl/patch-kb-drums-01` points to change `ulztwqmrxzvzxynuxpvxmmnxqzkpkrtz`, commit `7eeb8f6667f419762976315559d29063f42e181a`. The Release log is `/private/tmp/deluge-p44-velocity-drums-release.log`.
	- **Next:** The operator physically validates profiles, display feedback, interactions, and persistence. No firmware was flashed, installed, published, or submitted, and no Git command was used.

## Validation

- **Command or action:** Focused mapping and persistence checks, affected ARM objects, formatter, configured CTest suite, full diff review, and one logged Release build.
	- **Expected result:** New profiles work without changing legacy `FULL`, existing control gestures, or old-project loading.
	- **Observed result:** Passed. The profile spec executes nine scenarios; all 15 configured CTest targets pass; formatter and final adversarial review pass; the real ARM Release build compiled and linked the changed units in 212.88 seconds. The binary hash and log are recorded above.
	- **Failure signals:** Changed legacy velocity, mismatched brightness, a zero-velocity active cell in a new profile, Scale leakage, lost scroll or zoom, state-file incompatibility, or unrelated source changes.
	- **Interpretation:** Software checks establish deterministic mapping, input ownership, serialization wiring, target compilation, and legacy-path preservation. They do not establish physical control feel, LED brightness, or an actual save/reload cycle on the Deluge.

## Unfinished Work

- [ ] Physically confirm profile controls, quadrant placement, brightness, held-note behavior, Load and layout-change collisions, and save/reload on a Deluge before any device-verification or flashing decision.
