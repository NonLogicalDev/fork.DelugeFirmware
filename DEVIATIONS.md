# Local deviations from upstream Deluge Community Firmware

## Purpose and baseline

This document defines the complete local deviation stack for an agent beginning with a fresh checkout of upstream `main`. Recreate every contract below before considering that checkout equivalent to this local build. The entries describe outcomes, boundaries, and verification only. They intentionally do not prescribe source-level design, algorithms, or code.

The local stack currently contains four deviations:

1. Normalize the Korg MIDI-definition directory casing.
2. Provide a reproducible Apple Silicon Nix build environment.
3. Identify held chords in Keyboard View.
4. Show the latest physical held note under a recognized OLED chord label.

No entry authorizes firmware flashing, device installation, release publication, or upstream submission. Those actions remain manual and human-controlled.

## 1. Normalize Korg MIDI-definition directory casing

### Intent

Make the checked-out MIDI-definition tree consistent on case-sensitive and case-insensitive filesystems without changing any device definition.

### Required behavior

- The Korg manufacturer folders at `contrib/sd_card/MIDI` and `contrib/sd_card/MIDI_DEVICES/DEFINITION` use the spelling `Korg`.
- The former all-uppercase `KORG` folders are absent from the checkout.
- Every affected XML definition retains the same name, location below its manufacturer folder, and content apart from the parent directory's casing.

### Compatibility and verification

- MIDI device definitions, generated SD-card content, and firmware behavior are unchanged.
- A clean checkout contains exactly one Korg directory at each of the two affected locations.
- The working copy remains a full checkout. Do not use a sparse checkout as a workaround for the case collision.

## 2. Reproducible local Apple Silicon build environment

### Intent

Allow an Apple Silicon macOS developer to build this firmware locally from a pinned Nix environment rather than relying on an unmanaged cross-compiler installation.

### Required behavior

- The repository includes a locked Nix development environment under `.nix`, with brief human-readable setup guidance beside it.
- Entering that environment supplies the project build driver, Git support, formatting and pre-commit tools, Python helpers, and an official ARM GCC v22 cross-toolchain compatible with the Deluge's hard-float target.
- The environment selects that toolchain for firmware builds without requiring a global toolchain installation.
- A local Release build completes and produces the normal firmware binary at `build/Release/deluge.bin`.

### Compatibility and verification

- The environment is developer tooling only. It does not change firmware behavior or package a flashing workflow.
- The toolchain version and all Nix inputs are locked so a later agent can reproduce the same environment from a fresh upstream checkout.
- A Release build using the local environment passes with synchronizing disabled for the local build and does not publish or install an artifact.

## 3. Keyboard View exact chord recognition

### Intent

Give a player immediate feedback about a recognizable harmony while they hold notes in a melodic Keyboard View layout.

### Required behavior

- In a melodic Keyboard View layout, evaluate the currently held distinct pitch classes whenever the held-note set changes.
- Display a chord label only when the complete held pitch-class set exactly matches one non-empty chord shape already available in the firmware's chord vocabulary. Extra or missing pitch classes prevent a match; duplicate octaves do not change a match.
- Require at least three distinct pitch classes.
- Prefer the lowest held note as the root when it produces a valid chord name. When that is not valid, name a single unambiguous inversion. If multiple non-bass roots are equally valid, do not show a chord label.
- Format the label as the selected root plus the existing chord suffix, using the firmware's current sharp or flat naming preference.
- Show the label as a temporary OLED popup on OLED devices and as the existing concise scrolling chord label on seven-segment devices.
- Remove or restore the normal pre-existing note feedback when the player releases notes into a single note, an unrecognized set, or no held notes.

### Compatibility and boundaries

- Do not alter the chord vocabulary, chord-entry tools, note sound, note-on and note-off behavior, MIDI output, recording, or keyboard layout behavior.
- Do not add this recognition path to Kit/Drum output, the Chord layout, or the Chord Library layout. Those contexts have their own existing behavior and labels.
- Recognition is descriptive only. It must not create, transpose, quantize, record, or modify notes.

### Verification contract

- Automated tests cover a major chord across inversion and duplicate octaves, bass-root preference, a valid augmented chord, incomplete and unknown note sets, and an ambiguous naming case.
- The host unit-test suite and a local Release build pass.

## 4. Keyboard View chord and latest physical note display

### Intent

When exact chord recognition is active on an OLED, show both the harmony and the player's most recently pressed still-held note without changing how the instrument plays.

### Required behavior

- For a recognized chord on an OLED, show the existing chord label on the first line of its temporary popup.
- Show the most recently pressed physical note that remains held on the second line.
- Choose the second-line note by physical press order. Do not substitute the chord root, the lowest held note, a grid position, or a generated note.
- If the latest physical note is released while a recognized chord remains held, show the newest remaining physical note instead.
- Use the existing sharp or flat naming preference for the second-line note.
- If a recognized chord contains no physical held note, retain the one-line chord label rather than showing an invented or stale note.

### Compatibility and boundaries

- Seven-segment displays retain the one-line chord label introduced by the chord-recognition deviation. They do not gain a second-line representation.
- Recognition itself, unrecognized-set behavior, single-note feedback, sound generation, note-on and note-off behavior, MIDI output, recording, and layout behavior remain unchanged.
- The feature applies only where the chord-recognition deviation already applies.

### Verification contract

- Automated tests demonstrate that press order chooses the latest physical held note, that a release promotes the latest remaining physical note, and that a generated note cannot take precedence.
- The host unit-test suite and a local Release build pass.

## Maintaining this document

Each new local deviation adds or updates a contract in this file in the newest local feature revision. A completed document must remain understandable without local patch history, owner plans, or conversation context. It must identify the affected surface, exact required outcome, behavior that must remain stable, meaningful edge cases, and the evidence needed to verify the deviation. Do not add code, pseudocode, or implementation recipes.
