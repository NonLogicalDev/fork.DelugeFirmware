---
description: "Completed plan for a global five-press Shift gesture that invokes the existing Deluge Panic action."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Shift five-tap Panic gesture

## Purpose

Give the player a fast, menu-free way to silence runaway audio by pressing Shift five times in succession, while retaining the existing visible Settings-menu Panic action. This plan began on [[2026-08-22]].

## Product integration

- Existing model: Panic is a reusable global audio-stop action, currently exposed as the first item in the normal-operation Shift + Select Settings menu.
- User intent: reach that same emergency stop quickly from any ordinary operating view, without hunting through a menu while feedback or a runaway sample is loud.
- Integrated model: five consecutive quick Shift presses, with no intervening physical control press, invoke the existing Panic action. The gesture is recognized in the common button dispatcher, so it has one meaning across ordinary views and does not create per-screen variants.
- Existing behavior preserved: ordinary Shift modifiers, sticky Shift, menu access, and all existing Panic effects remain intact until the fifth qualifying press. The visible menu action remains available.

## Scope

- In scope: detect five Shift press edges no more than 500 ms apart globally; invoke the existing `AudioEngine::panic()` action; give the existing stopped feedback; reset the partial gesture after a timeout, another button or pad press, or successful Panic.
- Keep stable: normal Shift press-and-hold use, sticky Shift behavior, all ordinary controls, the current Settings-menu Panic entry, saved song data, settings, sequences, automation, undo history, raw output-buffer behavior, flashing, installation, publishing, and upstream submission.
- Out of scope: a new physical button, exposing Panic in special recording/export control screens beyond the existing routine, changing the audio-stop implementation, clearing the raw output buffer, or flashing firmware.

## Completion Criteria

- [x] Five Shift press edges no more than 500 ms apart, with no intervening button or pad press, invoke the same global Panic action.
- [x] Fewer presses, a slow sequence, or another button or pad press between Shift presses does not invoke Panic.
- [x] The fifth press provides the existing stopped feedback and resets the gesture so a sixth press starts a new sequence.
- [x] Existing Menu Panic behavior and normal Shift behavior remain available.
- [x] `DEVIATIONS.md` records the expanded Panic behavior as a self-contained contract.
- [x] Focused source review, host tests where the input seam permits, and a local Nix Release build pass. No flashing or publication occurs.

## Current State

- Status: completed locally; physical-device validation remains before any flashing decision.
- Decision: this belongs to the existing Panic core capability, not a new shortcut-specific component. It will receive the next `panic` bookmark, `nl/patch-panic-02`.
- Decision: interpret “in a row” as Shift-down edges no more than 500 ms apart with no intervening button or pad press. The fixed window uses the established short-press duration rather than the user-configurable Sticky Shift hold setting, which defaults to 100 ms and is unsuitable for a five-tap emergency gesture.
- Decision: the fifth press is consumed, clears sticky Shift, invokes Panic, and suppresses its matching release. With Sticky Shift enabled, the first four brief taps retain their existing toggle behavior; making them side-effect-free would require delaying ordinary Shift behavior until the gesture times out.
- Implementation: the common button dispatcher counts Shift-down edges after any card-routine deferral, resets on other button and pad presses, safely replays a completed fifth press after card work, clears Shift on Panic, and suppresses the fifth release so Sticky Shift cannot re-arm itself.
- Revision: `nl/patch-panic-02` at `1cc42bec`.
- Next action: a person with the Deluge can validate five-Shift Panic during active synth, Kit, audio-clip, preview, MIDI/gate, feedback, and effect-tail scenarios before making any flashing decision. Do not flash or publish.

## Work Log

- [[2026-08-22]]: The user requested Panic on five sequential Shift presses. Existing source inspection confirmed that Shift passes through the global `Buttons::buttonAction` dispatcher, while the established Panic routine already owns the full audio shutdown. The remaining design risk is preserving sticky Shift and modifier behavior.
- [[2026-08-22]]: `/root/panic_shift_input_survey` verified the common physical-input seam, card-routine deferral behavior, and the absence of an existing HID-button host test harness. It recommended five Shift-down edges within the established 500 ms short-press interval, reset by another button or pad press, and using the existing Panic routine plus stopped feedback. The detector will not include encoder turns, which are not button presses and would require a wider input change without a user need.
- [[2026-08-22]]: Implemented the five-Shift gesture in `src/deluge/hid/buttons.cpp` and expanded the self-contained Panic contract in `DEVIATIONS.md`. The configured Nix Release target linked `build/Release/deluge.elf`; the existing host CTest suite passed 7/7. The suite does not compile HID buttons, so a source review is still required to check the gesture's card-routine and sticky-Shift paths.
- [[2026-08-22]]: Independent review found that the first version counted a Shift press before a card-routine deferral, so the replayed event could count twice. Moved normal counting after the UI deferral return and retained a pending fifth-press path only for safely replaying an already completed gesture. The corrected Nix Release build linked `deluge.elf`; the configured host CTest suite again passed 7/7. The source has no dedicated HID-button host harness, so the review plus physical test cover that interaction path.

## Artifacts

- **A1:** Local Panic gesture source revision.
	- **Type:** Firmware source and local build artifact.
	- **Contents:** Five-Shift emergency entry, reset rules, card-routine replay handling, sticky-Shift cleanup, and self-contained behavior contract.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/src/deluge/hid/buttons.cpp` and `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.
	- **SCM surface:** Jujutsu revision `1cc42bec`, bookmark `nl/patch-panic-02`.

## Validation

- Source review: passed after correcting the card-routine replay double-count risk. It confirms reset-on-button/pad behavior, the 500 ms interval, fifth-release suppression, unchanged menu Panic, and safe fifth-press replay.
- Local Nix Release target: passed on [[2026-08-22]]; linked `build/Release/deluge.elf` and produced `build/Release/deluge.bin`.
- Host CTest suite: passed 7/7 on [[2026-08-22]]. The suite does not compile the HID input dispatcher, so it establishes broader host health rather than simulating this gesture.
- No physical device test, flashing, installation, publication, or upstream submission occurred.

## Unfinished Work

- Physical-device validation only: test exactly five rapid Shift presses, timeout and interruption resets, sticky-Shift cleanup, and silence across active voices, preview, feedback, and effect tails. Do not flash automatically.
