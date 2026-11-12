---
description: "Active plan to show the last pressed Keyboard View note under the recognized chord name."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Show chord and latest pressed note

## Purpose

Show the recognized chord and the most recently pressed note together in Keyboard View. This plan began on [[2026-08-22]].

## Scope

- In scope: retain the existing exact chord match, track the last physical Keyboard View note press, and render it below the chord name on OLED displays.
- Keep stable: note sound, note-on and note-off behavior, MIDI output, exact chord-matching rules, existing chord entry tools, the numeric-display chord label, and the local patch stack.
- Out of scope: flashing, installing, publishing, changing the physical Deluge, chord-progression features, or new music-theory recognition.

## Acceptance Criteria

- [x] A recognized chord on an OLED shows its chord name followed by the last physical note pressed on a second line.
- [x] The note label follows the actual most recent press, not the bass or a generated note, and updates safely as notes are released.
- [x] Unrecognized and single-note behavior remains as before; seven-segment displays retain the existing concise chord label.
- [x] Focused tests cover press order and the display-formatting helper or equivalent pure state seam.
- [x] `DEVIATIONS.md` records this behavior as an implementation-independent local-feature contract.
- [x] Unit tests and the local Nix Release build pass. No flashing or publication occurs.

## Current State

- Status: completed.
- Decision: the second line is the latest physical held note. The OLED popup's existing two-line text support provides the display without changing the seven-segment label.
- Result: `nl/patch-kb-chords-02` at `5484540aa1e8a7da1b9389402efeed9caac9b17b` contains the isolated local revision and root-level self-contained `DEVIATIONS.md` contract.
- Validation: 148 host unit tests passed. The local Nix Release build passed. `git diff --check` passed. No firmware was flashed, installed, published, or submitted upstream.
