---
description: "Active plan for local Keyboard View chord-name detection in the Deluge community firmware."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Keyboard View chord detection

## Purpose

Show the recognized chord when multiple notes are held together in Keyboard View. This plan began on [[2026-08-21]].

## Scope

- In scope: detect the currently held Keyboard View note set, reduce it to pitch classes, recognize exact catalogued chord qualities, and show a short chord-name popup. Correct the catalogued augmented-triad formula so standard augmented chords can be recognized.
- Keep stable: note sound, note-on and note-off behavior, MIDI output, existing chord entry tools, and the local `Korg` portability revision.
- Out of scope: flashing, installing, publishing, changing the physical Deluge, large music-theory libraries, or a general chord-progression feature.

## Acceptance Criteria

- [x] Holding a recognized simultaneous note set in Keyboard View shows its chord name without delaying or changing the notes played.
- [x] Releasing notes updates or clears the label correctly; repeated octaves do not change the detected chord.
- [x] Detection gives a matching bass priority, then accepts a unique inversion; ambiguous or unsupported sets fail quietly rather than guessing.
- [x] Focused tests cover recognized, inverted, duplicate-octave, bass-priority, standard augmented, and unsupported inputs.
- [x] The Nix build entry point validates the affected firmware target. No flashing or publication occurs.

## Current State

- Status: completed.
- Decision: use the existing chord catalog and `NoteSet` exact matching rather than a new theory library. Test the held bass first, then require a unique matching root for inversions. Chord and Chord Library layouts are excluded because they already display their own names.
- Decision: `kAug` currently contains a minor third, so it cannot recognize a standard augmented triad. Change it to a major third as the smallest necessary catalog fix, with a focused regression test.
- Result: local revision `ba152768ce07503a3ac376f764e562adac8dfc54` on bookmark `nl/patch-kb-chords-01` adds exact catalog matching and a Keyboard View label. It prefers the held bass, accepts only a unique inversion, restores the normal single-note label when needed, and leaves Kit, Chord, and Chord Library layouts unchanged. It also corrects the augmented-triad formula to major third plus augmented fifth.
- Validation: `UnitTests` passed (147 tests, 307351 checks). `DBT_NO_SYNC=1 ./dbt build Release` passed with the upstream v22 hard-float toolchain and produced `build/Release/deluge.bin`.
- Boundary: the binary is local only. Nothing was flashed, installed, pushed, or published.

## Work Log

- [[2026-08-21]]: Reordered the three local revisions as Korg, Nix environment, then chord detection. The content and successful validation of this patch are unchanged.
