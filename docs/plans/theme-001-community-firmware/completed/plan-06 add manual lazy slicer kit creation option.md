---
description: "Active plan to expose Manual/Lazy slicing directly when creating a Deluge slicer kit."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Manual/Lazy slicer kit creation option

## Purpose

Expose the existing Manual/Lazy slicing mode directly in the Slicer choice during kit creation. This plan began on [[2026-08-22]].

## Scope

- In scope: trace the kit-creation Slicer option and existing Manual/Lazy slicer behavior, then add a dedicated creation-time choice using that behavior.
- Keep stable: other kit creation choices, existing slicer defaults, sample handling, playback, saving, MIDI, and the local patch stack.
- Out of scope: flashing, installing, publishing, redesigning the entire kit-creation flow, or changing the actual Manual/Lazy slicing algorithm.

## Integration

- Existing model: the kit sample-browser menu offers Load all and Slice; Slice always opens the existing Slicer in its Region mode, while Manual mode is selected after entering it.
- Intent: let a player choose Manual slicing when they create a kit, without an extra mode change or a new kit type.
- Integrated model: add Manual slice as a sibling of Slice. It opens the same Slicer with its existing Manual behavior selected; Slice continues to open Region mode.
- Impact: kit creation menu, localized text, one-time Slicer entry mode, `DEVIATIONS.md`, and the local patch stack only.

## Acceptance Criteria

- [x] The kit-creation Slicer path exposes a dedicated Manual slice choice with localized OLED and seven-segment text.
- [x] The choice initializes the existing Manual mode for one entry only; standard Slice still initializes Region mode.
- [x] No host unit-test seam covers the Slicer UI lifecycle. Source review and the Release build cover the entry path; a physical runtime check remains human-owned.
- [x] `DEVIATIONS.md` records this behavior as an implementation-independent local-feature contract.
- [x] Unit tests and the local Nix Release build pass. No flashing or publication occurs.

## Current State

- Status: completed.
- Decision: use a localized Manual slice sibling action. It supplies the existing Manual mode only for that Slicer entry, then lets later normal entries retain their Region default.
- Result: `nl/patch-sample-slicer-01` at `844d748a53a32c92ee1baac18d7643d6de4d99e6` contains the isolated menu and entry behavior plus its `DEVIATIONS.md` contract.
- Validation: 148 host unit tests passed. The local Nix Release build passed and produced `build/Release/deluge.bin`. `git diff --check` passed. No firmware was flashed, installed, published, or submitted upstream.
- Follow-up: a person with the physical Deluge can confirm the three menu choices and both initial Slicer modes before any separate flashing decision.
