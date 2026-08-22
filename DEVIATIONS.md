# Local deviations from upstream Deluge Community Firmware

## Purpose and baseline

This document defines the complete local deviation stack for an agent beginning with a fresh checkout of upstream `main`. Recreate every contract below before considering that checkout equivalent to this local build. The entries describe outcomes, boundaries, and verification only. They intentionally do not prescribe source-level design, algorithms, or code.

The local stack currently contains nine deviations:

1. Normalize the Korg MIDI-definition directory casing.
2. Provide a reproducible Apple Silicon Nix build environment.
3. Identify held chords in Keyboard View.
4. Show the latest physical held note under a recognized OLED chord label.
5. Start kit creation in Manual Slice mode directly from the sample browser.
6. Improve Manual Slicer positioning and preview stopping.
7. Stop the browser preview on direct Manual Slice entry.
8. Organize local revisions by coherent firmware area or core capability.
9. Provide a global Panic action for immediate silence.

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

## 5. Manual Slice entry during kit creation

### Intent

Let a player choose manual slice placement before creating a kit, rather than entering the standard slicer and changing its mode afterward.

### Required behavior

- When the sample browser is creating a kit from a selected audio file, its action menu contains three choices: Load all, Slice, and Manual slice.
- Manual slice is available under the same selected-file condition as the existing Slice action. It is not offered for a folder.
- Choosing Manual slice opens the existing Slicer directly in its established Manual mode, with the same initial one-slice state, waveform view, pad editing, slice count, transpose, preview, save, confirm, and cancel behavior that Manual mode already provides.
- Entering either Manual slice or the existing Slice action requests the Slicer's initial grid redraw immediately. The player does not need to move an encoder or send another input before the waveform and Slicer grid appear.
- Choosing the existing Slice action continues to open the Slicer in its established Region mode with its existing initial slice count and controls.
- The Manual slice selection applies only to that entry into the Slicer. A later normal Slice entry must still begin in Region mode.
- The new menu label is localized for both OLED and seven-segment displays.

### Compatibility and boundaries

- Do not add a new slicing algorithm, slice-detection heuristic, kit type, sample format rule, maximum slice count, or save format.
- Do not change the behavior of Load all, standard Slice, the Slicer mode-switch control, existing manual slicing actions, sample playback, kit playback, MIDI, or project saving.
- The new choice is a faster route to an existing mode. It does not persist as a global default.

### Verification contract

- A human runtime check on a physical Deluge should confirm that Manual slice starts in Manual mode, standard Slice still starts in Region mode, and a normal entry after Manual slice is still Region mode.
- The host unit-test suite and a local Release build pass. The build remains local-only; flashing and installation are human-controlled.

## 6. Manual Slicer positioning and preview stop

### Intent

Make it practical to place manual slice boundaries across a large sample and silence an audition immediately without leaving the Manual Slicer.

### Required behavior

- In Manual Slicer mode, an ordinary Horizontal Encoder turn continues to move the selected slice start by 100 samples for each received detent.
- Holding the Horizontal Encoder down while turning it moves the selected slice start by 1,000 samples for each received detent. A faster physical turn already supplies a larger accumulated detent movement, so it covers more sample distance without changing the ordinary control. Shift alone does not select coarse movement.
- Turning the Horizontal Encoder or pressing another button or pad while it is held consumes that button press. Releasing it after a combined gesture does not switch Slicer mode or stop a pad audition. A press and release with no companion input retains the established Region and Manual mode switch wherever that switch is available.
- If storage work temporarily defers an encoder turn, the corresponding release must wait for normal input handling so the deferred turn still consumes the press instead of causing an accidental mode switch.
- Manual slice pads remain available for audition while the Horizontal Encoder is held, leaving the player's other hand free to select and audition slices during boundary editing.
- The existing ordering limits, sample-start and sample-end bounds, selected slice, slice count, and ordinary slice-point editing behavior continue to apply to both adjustments.
- Holding Shift and pressing Back stops the current Manual Slicer audition, shows the existing localized stopped feedback, and leaves the user in Manual Slicer with all slice data unchanged.
- After a Shift + Back stop, pressing a manual slice pad can audition that slice normally.

### Compatibility and boundaries

- The existing Save + Pad manual-slice deletion gesture remains the sole direct deletion gesture. This deviation does not add a Shift + Pad delete shortcut.
- Back without Shift retains its established behavior of leaving the Slicer. Shift + Back has no new behavior outside Manual Slicer mode.
- Do not change Region slicing, the slicing algorithm, sample data, kit playback, saving, MIDI, global undo history, firmware installation, or flashing behavior. In particular, Shift + Back must not cut sequenced kit voices that are unrelated to the current Manual Slicer audition.

### Verification contract

- Source review confirms that ordinary and pressed-encoder slice starts remain bounded and ordered for the first, middle, and last slices; a combined press gesture consumes the mode-switch tap even when encoder input is deferred; a bare tap retains mode switching; Shift + Back does not exit the editor or modify slice data; and pad audition remains available.
- The host unit-test suite and a local Release build pass. A person with a physical Deluge should confirm fine and coarse control feel, bare-tap mode switching, press-turn consumption, simultaneous pad audition, and the immediate stop behavior before flashing any build.

## 7. Stop browser preview on direct Manual Slice entry

### Intent

Enter direct Manual Slice ready for deliberate slice placement, without the selected sample continuing to audition from the browser.

### Required behavior

- Choosing Manual slice from the kit sample-browser action menu stops any current browser sample preview before the Manual Slicer opens.
- The selected sample remains available for Manual Slicer and its normal pad-based audition after entry.
- The Manual Slicer still opens in its established Manual mode with its normal initial state.

### Compatibility and boundaries

- The existing Slice action and Load all action retain their current entry and preview behavior.
- Stop only the dedicated browser preview. Do not stop sequenced kit voices, alter project playback, modify sample data, or change a saved project.
- Do not change the Slicer algorithm, its in-editor preview controls, its mode-switch behavior, MIDI, firmware installation, or flashing behavior.

### Verification contract

- A human runtime check on a physical Deluge should confirm that a playing browser preview is silent when Manual Slicer opens, that the first Manual Slicer pad can still audition the selected sample, and that standard Slice behavior is unchanged.
- The host unit-test suite and a local Release build pass. The build remains local-only; flashing and installation are human-controlled.

## 8. Local revision grouping

### Intent

Keep the local change stack understandable by grouping related revisions under one stable firmware area while preserving a clear name for a distinct core capability.

### Required behavior

- Local user-facing revisions use bookmarks named `nl/patch-<group>-<NN>`, where `NN` starts at `01` within that group.
- Related Manual Slicer work uses the `sample-slicer` group in stack order: direct Manual entry, editing controls, then direct-entry preview stopping.
- Related Keyboard View chord-display work uses the `kb-chords` group.
- A distinct core capability may use its own group. The Panic feature uses the `panic` group rather than the incidental Settings menu that exposes it.
- The repository-compatibility revision remains `local/fixups` at the bottom of the local stack, before user-facing revisions.
- When a new decision replaces earlier local behavior, fold it into the affected feature revision and update this document. Do not retain a later revision solely to cancel obsolete behavior, source, or contract text.
- Use Jujutsu, not Git commands, for local repository inspection, revision management, and diff review.

### Compatibility and verification

- This convention changes local revision names and organization only. It does not change firmware behavior, build outputs, installation, flashing, or upstream state.
- A local Jujutsu bookmark listing shows one sequential series for each firmware area or core capability, with no duplicate feature-specific group for the same area.
- The stack tip and this document contain only active local behavior. Superseded local behavior is absent rather than preserved as a follow-up cancellation.

## 9. Panic action

### Intent

Give the player one visible emergency action that immediately silences any source or effect that is currently making sound, including feedback loops and runaway samples.

### Required behavior

- The global Settings menu opened with Shift + Select presents Panic as its first action, before all existing Settings actions.
- Selecting Panic stops active playback, scheduled audio events, instrument notes, audio clips, MIDI and gate notes, auditions, and sample-browser preview sound.
- Panic removes active delay, stutter, modulation, compressor, filter, and reverb runtime state so no existing effect tail or feedback continues after the normal short output-buffer latency.
- The Panic routine immediately aborts active audio recording or stem export whenever a caller invokes it. An unfinished capture is intentionally not preserved, because emergency silence takes precedence.
- Panic gives the existing localized stopped feedback and keeps the Settings menu usable for a later action.

### Compatibility and boundaries

- The first Panic entry is available only through the normal-operation Settings menu. Recording and stem export retain their established controls until a later local deviation exposes the existing Panic routine in those modes.
- Panic changes only live audio and unfinished capture state. It does not change saved songs, presets, samples, sequences, automation, settings, MIDI configuration, or undo history.
- Panic does not directly clear the raw hardware output buffer. Any already queued output may finish during the normal short device latency; no new sound or effect tail may follow it.
- Existing transport controls, menu navigation, audio routing, and device installation behavior remain unchanged until Panic is selected.
- Panic is local-only. It does not authorize flashing, installation, publication, or upstream submission.

### Verification contract

- The host unit-test suite and a local Release build pass.
- A physical Deluge check verifies silence after invoking Panic during each currently reachable normal-mode scenario: a playing synth or kit, an audio clip, a sample-browser preview, a MIDI or gate note, delay feedback, stutter, modulation effects, reverb, a resonant filter, and compressor-driven audio. When a later caller exposes Panic during recording and stem export, validate the existing abort behavior in those modes too.
- The same check confirms no project content, settings, automation, or undo history changed after Panic outside the intentionally aborted unfinished capture.

## Maintaining this document

Each new local deviation adds or updates a contract in this file in the newest local feature revision. A completed document must remain understandable without local patch history, owner plans, or conversation context. It must identify the affected surface, exact required outcome, behavior that must remain stable, meaningful edge cases, and the evidence needed to verify the deviation. Do not add code, pseudocode, or implementation recipes.
