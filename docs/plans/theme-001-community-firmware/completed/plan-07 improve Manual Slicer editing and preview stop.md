---
description: "Active plan to improve Manual Slicer editing and add a direct preview-audio stop action."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Improve Manual Slicer editing and preview stop

## Purpose

Make manual sample slicing faster and safer. This plan began on [[2026-08-22]].

## Scope

- In scope: retain Save + Pad deletion and preview stop, keep plain Horizontal turns as the fine start-point control, and make Horizontal encoder press + turn the coarse control so the other hand remains free to audition pads.
- Keep stable: ordinary pad actions, regular scrolling, manual-slice ordering and limits, slice confirmation, standard Region slicing, kit playback, saving, and MIDI.
- Out of scope: changing the slicing algorithm, deleting sample audio, changing stored slice data without an explicit user action, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [x] The existing Save + Pad shortcut remains the only direct manual-slice deletion gesture.
- [x] Plain Horizontal turns retain the current fine start-point movement.
- [x] Holding the Horizontal encoder down while turning it moves the start point coarsely, with no Shift requirement.
- [x] The press-and-turn gesture works while the other hand auditions pads and does not also switch Slicer modes. A bare tap retains the existing mode switch where that switch is allowed.
- [x] A clear Manual Slicer action stops its preview audio without changing slice data or project playback.
- [x] `DEVIATIONS.md` records the behavior as a self-contained, implementation-independent contract.
- [x] `DEVIATIONS.md` describes the revised gesture without retaining the superseded Shift behavior.
- [x] Focused tests where the source seam permits, plus host unit tests and a local Nix Release build, pass. No flashing or publication occurs.

## Current State

- Status: completed.
- Findings: the Manual Slicer already maps each upper-grid pad to a slice, uses Save + Pad to delete, uses the Horizontal Encoder to change the selected slice start, and stops ordinary preview when its pad is released. The input handler receives an accumulated encoder delta, so a larger physical turn already supplies a larger adjustment. A source review rejected a first attempt that force-killed the kit's first drum because it could cut sequenced kit audio; the accepted behavior uses only the current audition's note-off path.
- Decision: supersede Shift + Horizontal as the coarse gesture. Plain Horizontal remains 100 samples per received detent; pressed Horizontal becomes 1,000 samples per received detent. Encoder press arms the established mode switch, any Horizontal turn consumes it, and release switches mode only after a bare tap. The encoder delta continues to scale movement naturally with turn speed.
- Result: `nl/patch-sample-slicer-02` at `c2d0838a11934f39048a91f19ceec3fdc8f28ee8` now uses plain Horizontal turns for 100-sample movement and pressed Horizontal turns for 1,000-sample movement. A combined turn, button, or pad gesture consumes the encoder tap; a bare tap retains mode switching where allowed. Deferred storage input cannot replay the release ahead of its turn, and independent neighbor clamps protect the first, middle, and last slice boundaries.
- Validation: independent review found and then cleared the deferred-release race and endpoint-clamp defect. Formatting passed, the configured host suite passed 7/7, and the Nix Release build produced `build/Release/deluge.bin` at 1,702,396 bytes with SHA-256 `84317cf92b575270cebdeae34b87eaaf893850db6c43eb0fa7758c102fb42c67`.
- Workers: `/root` completed integration; `/root/lazy_chop_controls` traced input ordering and audition interaction; `/root/lazy_chop_revision_tests` completed adversarial review and final re-review with no remaining defect.
- Follow-up: physical use found that the STOPPED feedback alone does not prove the Manual Slicer audition has ended. P15 now owns a correction that must provide an actual scoped stop before any later flashing decision.

## Implementation

- [x] Confirm Horizontal encoder press state is available during Manual Slicer rotation and preserve the established bare-tap action.
- [x] Rewrite `nl/patch-sample-slicer-02` with the revised modifier.
- [x] Update `DEVIATIONS.md` and local repository guidance.
- [x] Run review, host tests, and the local Nix Release build.
- [x] Leave hardware confirmation pending; do not flash.

## Work Log

- 2026-08-24 @10:34 — Reopened P11. The requested one-hand coarse control supersedes Shift + Horizontal and belongs in the existing `sample-slicer` revision.
- 2026-08-24 @10:46 — Folded the replacement into P11, corrected deferred-release ordering and endpoint clamps found in review, passed 7/7 host tests and the Release build, and left physical confirmation to the user. No Git, flashing, installation, or publication occurred.
