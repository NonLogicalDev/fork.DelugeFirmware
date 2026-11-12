---
description: "Active plan to make existing tracks visible as faint columns in the Deluge Song Grid."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Song Grid track column glow

## Purpose

Make the Song Grid easier to read at a glance: every visible column belonging to an existing track should have a very faint neutral-white underlay, while the existing active and inactive clip colors remain the clear visual focus. This plan began on [[2026-08-22]].

## Product integration

- Existing model: Song Grid columns represent tracks, and clip cells already communicate clip and playback state through their established colors and brightness.
- User intent: distinguish an existing instrument/track column from a truly empty column without making the grid busy or weakening clip-state feedback.
- Integrated model: track existence supplies a dim background layer; an existing clip continues to draw its current state on top. A column without a track remains dark.
- Architecture impact: confine the change to Song Grid rendering. Do not add state, persistence, a setting, new controls, or a second interaction model.

## Scope

- In scope: identify Song Grid's visible-column and clip-cell rendering seam; render a faint neutral-white base for visible columns with an existing output; preserve the current active and inactive clip appearance; update the local deviation contract and standing guidance; build and run the configured host suite.
- Keep stable: clip launch/selection, scroll and zoom mapping, scene rows, existing clip colors and brightness, arrangement/session behavior, track order, project data, MIDI, audio, Nix tooling, and hardware state.
- Out of scope: a user brightness preference, animation, changes to clip-state color logic, changes to other views, flashing, installation, publication, or upstream submission.

## Acceptance Criteria

- [x] Each visible Song Grid column backed by an existing track/output has a faint neutral-white underlay in otherwise empty cells.
- [x] A column without an existing track/output remains unlit.
- [x] Existing active and inactive clip colors and brightness remain visually dominant and functionally unchanged.
- [x] Grid interaction and coordinate mapping are unchanged.
- [x] `DEVIATIONS.md`, owner guidance, the configured host suite, and a local Nix Release build agree with the implemented behavior. No flashing occurs.

## Current State

- Status: active. Source implementation and local validation are complete; physical LED-balance confirmation remains. The Song Grid renderer clears its main pad image, maps visible columns through the existing track-count mapping, then draws each clip directly over that image.
- Revision: `nl/patch-song-grid-01` at `122353fb4d2653d7dc7e79165700b595d113a55f`.
- Worker: `/root` owns source tracing, implementation, validation, and Jujutsu revision management. No flashing or publication is authorized.
- Next action: on a physical Deluge, compare empty existing-track, empty non-track, inactive clip, active/playing clip, and scrolled columns. Confirm the 7/255 base remains visible but subordinate.

## Artifacts

- **A1:** Song Grid track-presence rendering.
	- **Type:** Firmware view rendering change.
	- **Contents:** Dim neutral-white column base for an existing visible track, layered beneath existing clip-state rendering.
	- **Location:** `src/deluge/gui/views/session_view.cpp`.
	- **SCM surface:** `nl/patch-song-grid-01` at `122353fb4d2653d7dc7e79165700b595d113a55f`.

- **A2:** Deviation contract.
	- **Type:** Self-contained behavior specification.
	- **Contents:** Visual hierarchy, stable interaction behavior, exclusions, and device verification requirements.
	- **Location:** `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/DEVIATIONS.md`.
	- **SCM surface:** `nl/patch-song-grid-01` at `122353fb4d2653d7dc7e79165700b595d113a55f`.

## Decisions

- **Decision:** Use existing track/output presence rather than the presence of a clip to choose the column base.
	- **Rationale:** The empty space beneath a real track is the information the user needs to see. A clip-only rule would disappear exactly where the new cue is useful.

- **Decision:** Make the base neutral-white and deliberately faint.
	- **Rationale:** It conveys structure without competing with the established clip colors or their active/inactive brightness states.

- **Decision:** Leave existing clip cells' rendering unchanged.
	- **Rationale:** Clip colors already carry meaningful state. Layering a base under blank cells avoids a second competing state system.

## Implementation Steps

1. [x] Trace the Song Grid renderer, its visible-column-to-output mapping, and the existing clip-cell draw order.
2. [x] Open a fresh `song-grid` Jujutsu revision and render the faint track base without changing clip-state drawing or interaction.
3. [x] Write the self-contained visual contract in `DEVIATIONS.md` and update standing owner/source guidance.
4. [x] Review the final diff, run the configured host suite and local Nix Release build, then assign `nl/patch-song-grid-01`.
5. [x] Record the device visual check; no flashing or installation is part of this plan.

## Validation

- **Command or action:** Local Nix Release build, configured host suite, source review, then a physical Song Grid visual check.
	- **Local result:** The Release build compiled `session_view.cpp`, linked `build/Release/deluge.elf`, and the configured CTest suite passed 7 of 7 tests. The final source review confirms the 7/255 base is applied before, and overwritten by, existing clip-state rendering.
	- **Expected device result:** Existing track columns have a barely visible neutral base; empty columns stay dark; clip-state cells retain their current apparent color and brightness.
	- **Failure signals:** A bright or colored background competes with clips, a base appears where no track exists, an existing clip looks dimmer or different, grid mapping changes, or build/test fails.
	- **Interpretation:** Local validation proves integration. A person with the physical Deluge must confirm the LED brightness balance before any flashing decision.

## Unfinished Work

- [x] Implement and locally validate the Song Grid rendering change.
- [ ] On a physical Deluge, compare empty, inactive-clip, active-clip, and playing-clip columns in Song Grid. Confirm the base is faint enough to stay behind all existing clip-state feedback.

## Work Log

- [x] 2026-08-22 22:25 - Created a separate Song Grid visual feature plan after completing the read-only hot-path audit. No firmware, build, or Jujutsu state changed.
- [x] 2026-08-22 22:50 - Added the 7/255 neutral base before existing Song Grid clip rendering, updated the self-contained contract and standing guidance, assigned `nl/patch-song-grid-01` at `122353`, linked the local Release ELF, and passed the configured host suite: 7 of 7 tests. No firmware was flashed.
