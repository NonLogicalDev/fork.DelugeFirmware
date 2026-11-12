---
description: "Read-only performance audit of Deluge firmware real-time and latency-sensitive paths."
tags:
  - note-type/project
  - status/growing
  - domain/tech
  - deluge
  - firmware
---

# Investigate real-time hot paths

## Purpose

Find practical ways to improve Deluge community-firmware performance without changing behavior by accident. Inspect the audio callback, voice and effect rendering, storage/card routines that can interrupt playback, MIDI/control dispatch, and high-frequency UI paths. Rank each concrete finding by user impact, confidence, implementation size, regression risk, and likely verification effort. This plan began on [[2026-08-22]].

## Product integration

- Existing model: local firmware changes already affect audio stopping, slicing, keyboard feedback, and MIDI input. The device must remain responsive while audio is rendering and while controls or card activity occur.
- User intent: understand where the real performance costs are and identify the best safe improvements before creating another behavior change.
- Integrated model: this is a read-only whole-codebase audit. Recommendations become separate feature plans and component-organized Jujutsu revisions only after the user chooses them.
- Architecture impact: classify work at the boundary where it belongs rather than treating all cost as an audio-engine problem. Preserve audio correctness, project data, MIDI behavior, and current local revisions.

## Scope

- In scope: static source audit, existing profiling hooks and build options, hot-path call graphs, dynamic allocation or blocking work on real-time paths, repeat work, avoidable copies, and rate mismatches.
- Keep stable: firmware behavior, local revision stack, `DEVIATIONS.md`, build environment, and hardware state. This plan authorizes no source optimization, build-system change, profiling firmware flash, or benchmark claim.
- Out of scope: speculative micro-optimizations without a demonstrated call path, premature rewrites, audio-quality tradeoffs, external profiling hardware, and upstream publication.

## Severity rubric

- **Critical:** likely to cause an audible dropout, runaway CPU use, missed deadline, or frozen control surface during normal use.
- **High:** repeatedly consumes enough time to reduce polyphony, delay a control response, or create a practical risk under ordinary songs.
- **Moderate:** measurable recurring work with limited user-visible effect until projects become dense.
- **Low:** infrequent, startup-only, or theoretical work that should not displace higher-value changes.

## Current State

- Status: completed. The read-only audit found no confirmed critical regression, but it identified several render-path allocation and overload-recovery candidates worth measuring or fixing.
- Constraint: an optimization must name its actual caller, execution frequency, expected win, affected behavior, and validation method. A line that merely looks expensive is not a finding.
- Worker: `/root` completed the read-only audit, evidence review, classification, and plan update. No source edit or Jujutsu revision was made.
- Next action: if the user wants an implementation, begin with a separate measurement revision or the selected GlobalEffectable lifecycle fix.

## Artifacts

- **A1:** Hot-path opportunity report.
	- **Type:** Plan findings and recommendation table.
	- **Contents:** Evidence-backed candidates with severity, expected benefit, fix size, risk, and suggested next action.
	- **Location:** This plan.
	- **SCM surface:** None during the read-only audit.

## Decisions

- **Decision:** Start with static call-path evidence, then state when hardware profiling is needed.
	- **Rationale:** The local environment can establish reachability, rate, allocation, and blocking risks without inventing performance numbers. Device measurements are required before claiming a measured improvement.

- **Decision:** Keep each eventual optimization in the component that owns its user-visible behavior.
	- **Rationale:** A focused audio, storage, MIDI-input, or UI revision is easier to review and validate than a broad performance patch.

- **Decision:** Do not change compiler flags as a first optimization.
	- **Rationale:** Release already uses `-O2`, LTO, and link-time devirtualization; DSP uses a unity build, ARM mode, and aggressive inlining. The source also records that broader compiler-flag experiments did not improve overall performance.

## Findings

| ID | Candidate | Severity | Fix ease | Evidence and likely benefit | Safe next step |
| --- | --- | --- | --- | --- | --- |
| F1 | Keep GlobalEffectable Mod FX and Grain memory out of the render call. | High | Medium | `GlobalEffectable::processFXForGlobalEffectable()` lazily allocates or frees the Mod FX buffer and can allocate a `GranularProcessor` while an output is rendering. The source calls this memory-for-real-time trade directly “dumb.” An effect depth/type transition can therefore add allocator work to the same window that must render audio. | Measure effect-on/off automation on a dense Kit, then keep or prepare the needed state before rendering and release it only at a safe non-audio point. Preserve effect tails and RAM-stealing behavior. |
| F2 | Move Delay buffer creation and resize out of the render path. | High | Hard | Every `Sound::render()` and `GlobalEffectable::createDelayWorkingState()` calls `Delay::setupWorkingState()`. It can allocate a low-speed buffer when delay activates and a second buffer when rate settling or a large rate change triggers a resize. This is a credible deadline spike while enabling or moving delay, but the allocation is tied to seamless audio continuity. | First collect timing around delay enable, rate sweeps, and tempo-synced changes. Design a pending prepared-buffer handoff before changing behavior. |
| F3 | Remove Sample Cache and Time Stretcher allocations from `VoiceSample::render()`. | High | Hard | The sample render path can allocate cache clusters. Time-stretch setup can acquire a stretcher and allocate its working buffer while rendering. Existing code limits hop work and falls back on allocation failure, but the transition cost can still coincide with audio. | Profile heavily stretched and repitched long samples with cache creation. Any fix needs an asynchronous/prepared state machine and must preserve cache priorities, click-free transitions, and culling. |
| F4 | Prepare live-input pitch-shift state before the first rendered buffer. | Moderate | Medium | `Voice::render()` can create `LiveInputBuffer` and `LivePitchShifter` on first pitched live-input use; `LivePitchShifter` can later allocate a repitched buffer. It affects a specific live-input workflow rather than ordinary synth/sample playback. | Prime the required state when the source/parameter becomes active, or queue it outside rendering. Test input-monitor latency and transitions between raw and pitched input. |
| F5 | Remove full Sound and Output scans from overload recovery. | Moderate under overload | Medium | Once the engine is already late enough to raise direness, `setDireness()` scans every Sound to count voices and every Output to count active Audio Clips before culling. The cull selection then scans voices again. This does not improve ordinary headroom, but it adds work exactly when recovery matters. | Establish a lifecycle-correct active-voice and active-audio-clip counter, or otherwise reuse cull traversal data. Stress dense Kits and repeated voice steals; do not make the counters another correctness risk. |
| F6 | Gate the release-only audio-latency clock reads. | Low | Easy | `routine_()` calls `getSystemTime()` twice every audio routine only to support a `D_PRINTLN` warning. Release disables text output, so the warning itself disappears but the time reads remain. The cost is probably small and needs measurement. | Guard the timing block with the existing text-output/debug condition, then compare device scheduler timing. Do this only with a measurement harness in place. |
| F7 | Add a non-default performance-measurement build switch. | Not a runtime defect | Easy | The scheduler already tracks per-task min/average/max duration and latency, but detailed output is compiled out. The code also has disabled audio-log and time-stretch-hop timers. The Release path already has the important optimization flags. | Add a build-only switch that enables bounded counters or periodic diagnostic output without changing Release behavior. Use it to rank F1-F6 on hardware before claiming speedups. |

## What is already protected

- The Release build uses the task manager and gives audio the highest priority. UI, OLED, PIC, and slow card tasks are separately scheduled below it.
- Audio uses bounded render windows of at most 128 samples, aligns normal windows to four samples for NEON, and only allows a limited number of new voices and time-stretch hops when under pressure.
- Audio, reverb, and per-output temporary buffers are fixed-size and reused. Their clears and mixes are real recurring work, but there is no source evidence that removing them is safe or worthwhile.
- Debug prints are compiled out in Release. The source already uses `-O2`, LTO, link-time devirtualization, and a DSP unity build, so a compiler-flag change is not a credible first win.

## Investigation Steps

1. [x] Map audio interrupt/render entry points, buffer cadence, voice iteration, global effects, and output mixing.
2. [x] Map card/storage routines, synchronization, allocation, logging, and UI/control code that can run while playback is active.
3. [x] Inspect existing profiling, timing, allocator, and compiler/optimization support before proposing new instrumentation.
4. [x] Record only candidates with an evidence path and classify each by severity, benefit, fix size, risk, and verification requirement.
5. [x] Re-read the ranked findings for local-revision and user-behavior boundaries; recommend at most a small first batch rather than a broad rewrite.

## Validation

- **Method:** Source-path review, existing comments/assertions/tests, and any already-present performance instrumentation.
	- **Expected result:** Each recommendation distinguishes verified fact from inference and names the required device measurement or stress test.
	- **Failure signals:** A candidate has no demonstrated hot-path reachability, a proposed change silently widens scope, or severity is based only on intuition.
	- **Interpretation:** This audit may identify candidates, not prove speedups. Measured claims require a separately authorized implementation and device-level comparison.

## Unfinished Work

N/A. The audit is complete. Any implementation requires a separate accepted plan and component-organized local revision.

## Work Log

- [x] 2026-08-22 22:00 - Created the read-only whole-codebase performance audit and severity rubric; no firmware, build, or Jujutsu state changed.
- [x] 2026-08-22 22:04 - Traced the audio engine, Song/output/voice/effect render chain, storage scheduling, task manager, current compiler settings, and disabled instrumentation. Published seven ranked, evidence-backed candidates; no firmware or build files changed.
