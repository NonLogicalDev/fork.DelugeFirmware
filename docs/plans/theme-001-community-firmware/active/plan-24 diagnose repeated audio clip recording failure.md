---
description: "Fix repeated same-Clip Audio Looper recording without losing the existing sample or corrupting recorder ownership."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Fix repeated Audio Looper recording failure

## Purpose

Make repeated Record + Play replacement passes safe in one Audio Clip whose Audio Output is `Looper/FX`. The first empty take may establish project tempo; every later pass must retain a valid Clip, install the completed replacement only after it is safe to do so, and never leave recorder ownership in a crash-prone state. This plan began on [[2026-08-24]].

## Product integration

- Existing model: `Looper/FX` is the Audio Clip mode that continuously monitors microphone or line input. Internally, that mode also records a later pass into the same Clip instead of cloning an overdub Clip.
- User intent: repeat Record, Play, then Play to stop in the same Audio Looper Clip. A completed pass replaces the Clip's sample. A cancelled pass or a recorder rejected before replacement preserves the prior sample. Neither path may make the Clip disappear or crash later.
- Integrated model: only an empty first Audio Clip may use the tempoless path that establishes tempo. Once the Clip contains a sample, replacement recording uses the established song clock. The replacement recorder remains valid through finalization, old-sample cleanup, and sample installation, then detaches exactly once from the Output that owns it.
- Architecture impact: the repair belongs in shared Session, AudioClip, Output, and SampleRecorder lifecycle rules rather than a Song-layout adapter. The separate cloned-overdub path remains outside this fix.

## Scope

- In scope: tempoless-recording eligibility for an empty first Audio Clip; repeated in-place recording for `Looper/FX`; safe replacement of an existing AudioClip sample; identity-safe SampleRecorder attachment and detachment; focused regression coverage; Release build; the local behavior contract in `DEVIATIONS.md`; and one separate Jujutsu bugfix revision.
- Keep stable: first-take tempo establishment, continuous input monitoring in `Looper/FX`, Clip activation and location, ordinary playback, non-Looper recording and overdub behavior, existing undo history, Row and Grid controls, current local patch stack, and saved project data.
- Out of scope: the separate held-Clip plus short-Record cancellation defect from P27; broader continuous-overdub or Grid behavior changes; new UI; firmware flashing; publication; and hardware validation that requires the user's Deluge.

## Reproduction reported

1. In Audio Clip View, set Audio Output to `Looper/FX`.
2. In the first empty Clip, press Record, then Play, then Play again to stop. The take remains and establishes tempo.
3. Repeat Record, Play, then Play to stop in that same Clip.
4. The Clip content disappears; repeating the cycle again can crash the device.

## Proven failure

- Session admits the non-empty in-place Audio Clip to the first-take tempoless path, so the second pass runs without the established internal clock.
- During replacement finalization, AudioClip cleanup sees the existing sample, clears it, and aborts the same recorder that finalization is still using. The Clip remains but its sample, path, and name are empty; the next recorder access can dereference null.
- SampleRecorder teardown has a second ownership hazard: it can ask an Output to remove a recorder without proving that the Output still owns that exact recorder. A failed second attachment can detach the first recorder.

## Completion Criteria

- [x] A non-empty Audio Clip cannot re-enter the first-take tempoless path; the empty first take still establishes tempo.
- [x] Finishing a same-Clip `Looper/FX` replacement cannot abort or invalidate the recorder being finalized.
- [x] A completed replacement installs the new sample without deleting or deactivating the Clip; cancellation, no captured samples, a file-size rejection, or recorder-attachment rejection preserves the previous sample.
- [x] Recorder detachment is null-safe, idempotent, and identity-safe: destroying or detaching one recorder cannot remove another recorder from an Output.
- [x] Focused tests cover lifecycle behavior where the host harness can represent the real objects. Any missing seam is recorded explicitly and covered by source review, the Release build, and a physical repetition script rather than a test-local imitation.
- [x] The configured host tests and Release firmware build pass from the repository's Nix environment.
- [x] `DEVIATIONS.md` describes the observable fix as a self-contained behavioral contract without implementation instructions.
- [x] The finished change has one local bookmark using the requested `bugfix-<slug>-<NN>` convention. No Git, flash, push, or publication occurs.

## Blockers

- A physical Deluge is required to confirm the complete microphone or line-input loop after the local build. This does not block source repair, host regression tests, or the firmware build.

## Artifacts

- **A1:** Repeated-audio-recording lifecycle report.
	- **Type:** Source-backed investigation in this plan.
	- **State:** Cause established; retained above under Proven failure.
	- **Location:** This plan.
- **A2:** `ns/bug-audio-looper-01`.
	- **Type:** Planned local Jujutsu bugfix revision and bookmark.
	- **State:** Local change `rkmozsms` is bookmarked `ns/bug-audio-looper-01` above `nl/patch-song-grid-01` after review and validation.
	- **Contents:** Narrow lifecycle repair, focused tests, and updated `DEVIATIONS.md`.
	- **Publication:** Local only.

## Workers

- `/root/looper_core`: completed lifecycle and storage-boundary review.
- `/root/looper_ownership`: completed exact-recorder Output ownership and detachment.
- `/root/looper_tests`: completed host-test seam assessment and two adversarial source and contract reviews.

## Decisions

- **Decision:** Keep this separate from P27 Grid-versus-Row looping.
	- **Rationale:** The reproduction and defect are in shared Session, AudioClip, Output, and recorder lifecycle code. Grid is neither required for reproduction nor the correct repair boundary.
- **Decision:** Treat `Looper/FX` continuous monitoring and same-Clip recording as established behavior.
	- **Rationale:** The fix must preserve why this Audio Clip mode exists; it may only repair lifecycle classification and ownership.
- **Decision:** Fix both the immediate finalization order and the recorder's Output ownership rule.
	- **Rationale:** Preventing the immediate null access alone would leave a failed or stale recorder able to detach a different recorder later.
- **Decision:** Use one bugfix revision rather than modifying `nl/patch-song-grid-01`.
	- **Rationale:** The two changes have different behavior, source ownership, validation, and rollback boundaries.

## Implementation Steps

1. [x] Map the empty first take and later same-Clip `Looper/FX` pass through Session, Playback, AudioClip, Output, and SampleRecorder.
2. [x] Prove the first divergence and the later ownership hazard from source.
3. [x] Create a fresh Jujutsu change above `nl/patch-song-grid-01`.
4. [x] Restrict first-take tempoless eligibility and make replacement finalization preserve its recorder.
5. [x] Make recorder attachment and detachment identity-safe.
6. [x] Assess focused regression coverage and run the host suite and Release build.
7. [x] Update `DEVIATIONS.md`, review the complete diff, and create the local bugfix bookmark.

## Work Log

- [x] 2026-08-24 02:18 - Created after a reported second-pass audio Clip disappearance and repeat-path crash.
	- **Evidence:** First loop creates a tempo successfully, but a later identical Record + Play attempt can delete the Clip and eventually crash.
	- **Effect:** No source, Jujutsu, build, or device state changed.
	- **Next:** Trace shared Playback, Session, AudioClip, and AudioOutput lifecycle paths.
- [x] 2026-08-24 - Confirmed the exact reproduction uses one Audio Clip with Audio Output set to `Looper/FX`.
	- **Evidence:** The user clarified that `Looper` designates continuous microphone or line-input monitoring and accepted the same-Clip recording interpretation.
	- **Effect:** The fix must preserve continuous monitoring and in-place recording rather than remove the mode's intended behavior.
	- **Next:** Repair the lifecycle, not the user-facing mode.
- [x] 2026-08-24 - Proved why the second pass disappears and can crash.
	- **Evidence:** Session admits a non-empty in-place Audio Clip to the tempoless path; finish then clears the old sample, and the no-clock cleanup aborts and nulls the same recorder that finalization next reads. SampleRecorder teardown also lacks an exact-owner check.
	- **Effect:** The accepted fix has three contracts: empty-only tempoless eligibility, safe replacement finalization, and identity-safe recorder detachment.
	- **Next:** Implement these contracts in a separate bugfix revision and validate repeated replacement and cancellation.
- [x] 2026-08-24 - Created isolated Jujutsu working change `rkmozsms` above `nl/patch-song-grid-01` and assigned the three repair surfaces separately.
	- **Evidence:** `jj status` identifies the working change as `fix repeated Audio Looper recording`; the parent retains `nl/patch-song-grid-01`.
	- **Effect:** The audio fix remains independently reviewable and reversible without changing the Song Grid patch.
	- **Next:** Integrate the three narrow changes, then run adversarial review and validation.
- [x] 2026-08-24 - Completed the source repair, two review passes, contract audit, host suite, Release build, and local bookmark.
	- **Evidence:** Formatter check passed; CTest passed 7 of 7; `build/Release/deluge.bin` linked at 1.6 MB with SHA-256 `2acdc03c06150d18e9aa26a7c952e14d159e1589a170bbdd562a9f82a9bb5c13`; Jujutsu shows `ns/bug-audio-looper-01` on change `rkmozsms`.
	- **Effect:** The local fix is independently reviewable above `nl/patch-song-grid-01`. No Git, flash, push, or publication occurred.
	- **Next:** Run the physical first-take, ten-replacement, cancellation, and zero-capture script before deciding whether to flash this build.

## Validation

- **Check:** Focused host regression tests for lifecycle classification and recorder ownership.
	- **Expected result:** Empty first take remains eligible; non-empty replacement is clocked; a completed replacement installs once; a cancelled replacement preserves the old sample; detaching or destroying an unattached recorder cannot disturb an attached recorder.
	- **Observed result:** The host target does not compile the real Session, AudioClip, Output, or SampleRecorder lifecycle. No duplicate-model test was added; an independent source review traced every affected transition and caller instead.
	- **Failure signals:** Any repeat pass is tempoless, old content is cleared before a replacement is safely owned, or a recorder removes a different Output recorder.
- **Check:** Repository host test suite from `nix develop path:./.nix`.
	- **Expected result:** All configured tests pass.
	- **Observed result:** Passed 7 of 7 CTest targets; the UnitTests target reports 151 tests and 307,399 checks.
- **Check:** Release firmware build with checkout-local ccache enabled.
	- **Expected result:** Release firmware links successfully with no new warnings attributable to this change.
	- **Observed result:** Passed. `build/Release/deluge.bin` is 1.6 MB with SHA-256 `2acdc03c06150d18e9aa26a7c952e14d159e1589a170bbdd562a9f82a9bb5c13`. The linker emitted its generic RWX load-segment warning.
- **Check:** Physical Deluge repetition using microphone or line input in `Looper/FX`.
	- **Expected result:** At least ten same-Clip record/stop cycles retain the Clip, replace only after success, keep monitoring coherent, and never freeze or crash.
	- **Observed result:** Pending user hardware test after local source and build validation.
	- **Interpretation:** Local checks prove source consistency and buildability; only the device test proves the complete hardware workflow.

## Unfinished Work

- [ ] Run the short physical regression script; do not claim hardware validation before the user completes it.
- [ ] Treat late storage-write failure preservation as a separate transactional-storage change if the user wants it; this revision intentionally does not promise it.
