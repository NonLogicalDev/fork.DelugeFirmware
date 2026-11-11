# Changes from upstream Deluge Community Firmware

## Purpose and baseline

This document specifies this fork's changes to upstream `main`. It is written for someone recreating the features from a fresh upstream checkout, without this fork's history or the conversations behind it. An equivalent build must satisfy every requirement below.

Each entry describes the intended behavior, compatibility requirements, exclusions, and checks. The checks are acceptance criteria, not claims that hardware testing is complete. Implementation is open: this document does not prescribe code, algorithms, or source structure.

The fork contains thirty deviations:

1. Normalize the Korg MIDI-definition directory casing.
2. Provide a reproducible Apple Silicon Nix build environment.
3. Identify held chords in Keyboard View.
4. Show the latest physical held note under a recognized OLED chord label.
5. Start Kit sample creation and reuse directly from the sample browser.
6. Improve Manual Slicer positioning and preview stopping.
7. Stop the browser preview on direct Manual Slice entry.
8. Organize local revisions by coherent firmware area or core capability.
9. Provide a global Panic action for immediate silence.
10. Copy, paste, and reorder a Kit Sound Drum row.
11. Add optional incoming MIDI sustain-pedal support for internal Synth tracks.
12. Show existing Song Grid track columns with a faint neutral base.
13. Protect repeated recording into one Audio Clip in `Looper/FX` mode.
14. Make Audio Clip waveforms follow their Song Grid track color.
15. Keep waveform peak addressing valid throughout the supported long-file range.
16. Keep reversed single-row waveform ranges inside their half-open display bounds.
17. Show efficient OLED waveform companions while editing sample bounds and slices.
18. Show a compact OLED Clip timeline ruler.
19. Audition the exact Kit row being edited in Waveform Editor.
20. Make Audio Clip Start editing reversible from Audio Clip View.
21. Make Waveform Editor entry and bound selection explicit.
22. Add selectable velocity profiles to Velocity Drums.
23. Let Session MIDI-Out Clips advance from independent external steps.
24. Make held-Horizontal preview-only note input release-order safe across Clip and Keyboard views.
25. Keep playheads legible on full-screen waveform displays.
26. Refuse incompatible Songs before changing playback or project state.
27. Add independent persistent choke groups to Kit Sound Drum rows.
28. Maintain an incremental, searchable keybinding catalog beside the firmware source.
29. Open a shared Song Root / Mode menu with Shift + Scale.
30. Edit envelope times with coarse and fine resolution.

The following rule applies to every entry: implementing or checking a contract does not authorize flashing, device installation, release publication, or upstream submission. Those actions require separate human direction; flashing and device checks require a person with the Deluge.

## 1. Normalize Korg MIDI-definition directory casing

### Purpose

Make the MIDI-definition tree work on both case-sensitive and case-insensitive filesystems without changing device definitions.

### Behavior

- Use `Korg` for the manufacturer folder at both `contrib/sd_card/MIDI` and `contrib/sd_card/MIDI_DEVICES/DEFINITION`. Remove the former `KORG` folders.
- Preserve every affected XML file's name, content, and path below its manufacturer folder.
- Keep the full checkout; do not work around the case collision with a sparse checkout.

### Checks

- A clean checkout has exactly one Korg directory at each location.
- MIDI definitions, generated SD-card content, and firmware behavior remain unchanged.

## 2. Reproducible local Apple Silicon build environment

### Purpose

Let an Apple Silicon macOS developer build the firmware with a pinned Nix environment, without installing a global cross-compiler.

### Behavior

- Provide a locked development environment under `.nix`, with brief setup instructions beside it.
- Include the project build driver, Git support, formatting and pre-commit tools, Python helpers, and an official ARM GCC v22 cross-toolchain compatible with the Deluge's hard-float target.
- Select that toolchain for firmware builds. Lock its version and all Nix inputs so a fresh checkout can reproduce the environment.
- Cache C and C++ compilation under the checkout's ignored `.cache/ccache` directory. Cache misses use the same locked v22 toolchain.
- Keep the cache disposable and local to that checkout. Do not commit, upload, share, or treat it as a firmware artifact.
- This environment supplies developer tools only; it changes no firmware behavior and includes no flashing workflow.

### Checks

- A newly configured build uses `ccache` as its C and C++ compiler launcher. An existing build does the same after one reconfiguration.
- Cache statistics are inspectable after a build; an initial build may contain only misses.
- A Release build in this environment, with synchronizing disabled, produces `build/Release/deluge.bin` without publishing or installing it.

## 3. Keyboard View exact chord recognition

### Purpose

Name a recognizable chord while the player holds notes in a melodic Keyboard View layout.

### Behavior

- Re-evaluate the distinct held pitch classes whenever the held-note set changes.
- Require at least three distinct pitch classes and an exact match to a non-empty shape in the firmware's existing chord vocabulary. Missing or extra pitch classes prevent a match; duplicate octaves do not affect it.
- Prefer the lowest held note as the root if it gives a valid name. Otherwise, name an unambiguous inversion. Show no chord label if multiple non-bass roots are equally valid.
- Combine the root with the existing chord suffix, respecting the current sharp or flat naming preference.
- Use a temporary popup on OLED and the existing concise scrolling chord label on seven-segment displays.
- When only one note, an unrecognized set, or no notes remain held, clear the chord label and return to the normal note feedback for that state.

### Compatibility

- Recognition only describes held notes. It does not create, transpose, quantize, record, or modify them.
- Preserve the chord vocabulary, chord-entry tools, sound, note-on and note-off behavior, MIDI output, recording, and layouts.
- Exclude Kit/Drum output, Chord layout, and Chord Library layout; these retain their existing behavior and labels.

### Checks

- Automated tests cover major-chord inversions and duplicate octaves, bass-root preference, a valid augmented chord, incomplete and unknown sets, and ambiguous naming.
- The host unit-test suite and a local Release build must pass.

## 4. Keyboard View chord and latest physical note display

### Purpose

Show the recognized chord and the most recently pressed note still physically held, together in the OLED popup.

### Behavior

- Keep the chord label on the first line. Put the latest physical held note on the second, using the current sharp or flat naming preference.
- Choose that note by physical press order, not chord root, lowest pitch, grid position, or generated-note order.
- If the latest note is released and a recognized chord remains, show the most recently pressed physical note still held.
- If a recognized chord has no physical held notes, show only the chord label. Do not show a stale or invented note.

### Compatibility

- Apply this only in the contexts covered by exact chord recognition.
- Seven-segment displays keep their one-line chord label.
- Preserve recognition, unrecognized-set and single-note feedback, sound, note-on and note-off behavior, MIDI output, recording, and layouts.

### Checks

- Automated tests cover physical press order, promotion of the latest remaining note after release, and exclusion of generated notes from that choice.
- The host unit-test suite and a local Release build must pass.

## 5. Kit sample-creation entry and reuse

### Purpose

Add a folder, region slices, or manual slices at the top of a Kit, starting at the selected Sound Drum and leaving lower rows alone.

### Behavior

#### Entry and placement

- The action menu for a selected audio file during Kit creation offers Load all, Slice, and Manual slice. Localize the new label for OLED and seven-segment displays.
- In a brand-new Kit, offer Manual slice under the same selected-file condition as Slice, never for a folder.
- In an existing Kit, allow all three actions when the selected pad contains a Sound Drum and no assigned row is above it. That selected drum is the anchor: it receives the first generated item even if it already has a sample. Append later items above it.
- Reject a selected MIDI or Gate Drum, or a Sound Drum with an assigned row above it. Use the localized feedback that a top empty Kit pad is required.
- This is the only route for reusing an existing Kit. Do not insert into its middle or move, replace, clear, or reorder rows below the anchor. Preserve their notes, sound settings, and playback state.
- Before either Slicer mode opens in an existing Kit, make the selected file the anchor's playable source. Use it for the waveform and audition, never an earlier browser preview.
- Manual slice stops the separate browser preview and opens the existing Manual mode. Keep its one-slice initial state, pad editing, slice count, transpose, save, confirm, and cancel behavior.
- Slice opens Region mode with its existing initial slice count and controls. Load all keeps filename-based row names and uses the same anchor-and-above placement.
- Draw the waveform and Slicer grid immediately on either entry; no encoder movement or further input is needed.
- An existing-Kit Slicer session stays in its entry mode. Its mode-switch control cannot change modes and invalidate the anchor or source. Preserve the mode-switch control in a brand-new Kit.
- Manual entry affects only that session. A later ordinary Slice entry still starts in Region mode.

#### Batch mode and confirmation

- Confirming two or more slices creates exactly one playable Sound Drum per requested slice in a new or existing Kit. Every row retains its assigned bounds, transpose, mode, and batch name; confirmation must not lose later rows.
- Under the default duration rule, slices longer than two seconds use the configured default sample mode. Slices shorter than two seconds use Once so short fragments do not choke later hits.
- The top three pads of the far-right status column select Default, All Cut, and All Once, in that vertical order. Brighten the selected pad and show immediate display feedback. Preserve the main slice grid and far-right audition column.
- Default follows the duration rule. All Cut assigns Cut to every slice in the pending batch, including short slices; All Once assigns Once to every slice. These overrides affect neither the stored device default nor existing rows.
- Select opens an Auto, Cut, Once confirmation menu in Region and Manual Slicer. On either display type, highlight the pending mode selected by the right-side pads.
- Turning Select changes only the highlight. Pressing Select accepts it, closes the menu, updates the same pending batch mode as the pads, and immediately runs slice confirmation.
- Back closes the menu without changing the source, mode, boundaries, slice count, transpose, preview, or pending batch mode.
- If confirmation fails, keep Slicer open with the accepted mode, existing session, and existing error feedback.
- Manual audition may temporarily use Once. Cancelling restores the anchor's prior mode; only confirmation commits the batch mode.
- Back from Manual Slice leaves the new anchor as one full-sample pad, as before. Other existing Kit rows remain unchanged.

#### Names

- Name the first confirmed Slice or Manual slice batch `A-01`, `A-02`, and onward. Use a zero-padded part number.
- Each later batch uses the series after the highest existing generated series: `B-01`, then `C-01`, with `AA-01` after `Z-01`.
- Preserve existing row names. A pre-existing name in generated-series form reserves that series; a legacy numeric slice name does not.
- Assign names only on confirmation. Manual preview and cancellation never rename a row.

### Compatibility

- Do not change the slicing algorithm, detection heuristics, Kit type, sample format rules, maximum slice count, save format, Kit playback, MIDI, or project saving.
- Batch-mode controls affect only the batch about to be confirmed while Slicer is open. Other sample-loading paths and device defaults remain unchanged.
- Manual slice is a shortcut to an existing mode, not a persistent global default.

### Checks

- On a physical Deluge, hold Select on an audio file and confirm the Manual slice action is available without a timing-sensitive gesture. Its waveform and pads must audition that file. Ordinary Slice must start in Region mode, including after a Manual session.
- Start a second batch from a valid top Sound Drum using a different file. Confirm the new file supplies both waveform and sound, not a tail or preview from the first batch.
- Check Load all, Slice, and Manual slice from a valid top empty pad. The anchor receives item one, later items appear above it, and lower rows remain unchanged. Reject an anchor with an occupied row above.
- In both Slicer modes and on both display types, check that the confirmation menu starts at the right-pad choice, turning changes only the highlight, and Back leaves the session unchanged.
- Accept each mode with long and short slices and check playback behavior. Check that a failed confirmation retains the session and accepted mode with its existing error, and cancelled Manual audition restores its prior mode.
- Confirm distinct series names for two batches, unchanged names during Manual preview and cancellation, and preserved names after save/reload.
- The host unit-test suite and a local Release build must pass.

## 6. Manual Slicer positioning and preview stop

### Purpose

Move manual slice starts finely or coarsely while auditioning with the other hand, and stop an audition without leaving Slicer.

### Behavior

- Ordinary Horizontal Encoder turns move the selected slice start by 100 samples per received detent. Holding the encoder while turning moves it by 1,000 samples per detent.
- Faster physical turns already accumulate more detent movement and therefore cover more distance. Shift alone does not select coarse movement.
- A turn, button press, or pad press while Horizontal is held consumes the encoder's tap. Releasing it after that combined gesture neither switches Slicer mode nor stops a pad audition.
- A bare press and release keeps the Region/Manual mode switch wherever that switch is available.
- If storage work defers a turn, defer its release through normal input handling too. The delayed turn must consume the tap rather than cause an accidental mode switch.
- Keep manual pads available for selection and audition while Horizontal is held.
- Both movement sizes obey the existing slice order and sample bounds, without changing the selected slice, slice count, or other slice-point editing behavior.
- Shift + Back stops only the current Manual Slicer audition, shows the existing localized stopped feedback, and stays in Slicer with all slice data intact. A later pad press auditions normally.

### Compatibility

- Save + Pad remains the only direct slice-deletion gesture; do not add Shift + Pad deletion.
- Plain Back still leaves Slicer. Shift + Back gains no behavior outside Manual mode.
- Preserve Region slicing, the algorithm, sample data, Kit playback, saving, MIDI, and global undo history. In particular, stopping Manual audition must not cut unrelated sequenced Kit voices.

### Checks

- Review bounds and ordering for the first, middle, and last slice at both movement sizes. Check that combined gestures consume taps even when a turn is deferred, bare taps still switch modes, Shift + Back preserves the editor and slice data, and pads remain playable.
- On a physical Deluge, check fine and coarse feel, bare-tap mode switching, press-turn consumption, simultaneous pad audition, and immediate stopping.
- The host unit-test suite and a local Release build must pass.

## 7. Stop browser preview on direct Manual Slice entry

### Purpose

Enter Manual Slice silently so the player can audition slices deliberately.

### Behavior

- Choosing Manual slice in the Kit sample-browser action menu stops the dedicated browser preview before opening Manual Slicer.
- Keep the selected sample available for pad audition. Open Manual mode with its normal initial state.

### Compatibility

- In a brand-new Kit, Slice and Load all retain their entry and preview behavior. For Slice in an existing Kit, follow the source rules in section 5.
- Stop no sequenced Kit voices and change no project playback, sample data, or saved project.
- Preserve the Slicer algorithm, in-editor preview controls, mode switching, and MIDI.

### Checks

- On a physical Deluge, confirm browser preview is silent on entry, the first Manual pad auditions the selected sample, and ordinary Slice behavior is unchanged.
- The host unit-test suite and a local Release build must pass.

## 8. Local revision grouping

### Purpose

Group revisions by firmware area, with a separate group for each distinct core capability.

### Behavior

- Name feature bookmarks `nl/patch-<group>-<NN>`, numbering from `01` within each group.
- Give confirmed defects that predate the local stack their own `ns/bug-<component>-<NN>` bookmarks. Related defects share a component slug and increment its number.
- Fold a defect introduced by a local feature into its original `nl/patch-*` revision, not a later bug bookmark.
- Use `sample-slicer` for direct Manual entry, editing controls, and direct-entry preview stopping, in that order.
- Use `kb-chords` for Keyboard chord-display work and `kit-rows` for Sound Drum row-copy work.
- A distinct core capability may use its own group. Panic uses `panic`, not the Settings menu that happens to expose it.
- Keep `local/fixups` first in the stack, before user-facing changes.
- Fold replacement behavior into the affected feature revision and update this document. Do not keep a later revision solely to cancel obsolete behavior, source, or contract text.
- Use Jujutsu, not direct Git commands, for local inspection, revision management, and diff review.

### Checks

- The Jujutsu bookmark listing has one numbered series per feature area, core capability, or pre-existing bug component, without duplicate groups for the same area.
- The stack tip and this document contain only intended behavior, not superseded behavior followed by a cancellation.
- Naming and organization alone change no firmware behavior, build output, installation, flashing, or upstream state.

## 9. Panic action

### Purpose

Silence every current sound source and effect, including runaway samples and feedback.

### Behavior

- Put Panic first in the normal Settings menu opened with Shift + Select.
- Also invoke it on five physical Shift presses, each starting no more than 500 ms after the previous press, with no intervening button or grid-pad press. Encoder turns do not affect this gesture.
- The first four presses retain normal Shift behavior. The fifth invokes Panic, shows stopped feedback, starts a new gesture sequence, and leaves Shift off.
- Fewer than five presses, a gap over 500 ms, or an intervening button or pad press must not invoke Panic.
- Stop playback, scheduled audio events, instrument notes, audio clips, MIDI and gate notes, auditions, and browser preview.
- Clear active delay, stutter, modulation, compressor, filter, and reverb state. No existing tail or feedback may continue beyond normal short output-buffer latency.
- Whenever invoked, abort active audio recording or stem export immediately. Discard unfinished capture; emergency silence takes priority.
- Give the existing localized stopped feedback. Keep Settings usable for the next action.

### Compatibility

- The menu action is available during normal operation. Five Shift presses provide a global physical-button gesture.
- Recording and stem export retain their existing controls; this contract adds no dedicated menu action in those modes.
- Change only live audio and unfinished capture state. Preserve saved songs, presets, samples, sequences, automation, settings, MIDI configuration, and undo history.
- Do not directly clear the raw hardware output buffer. Already queued output may finish during normal short latency, but no new sound or effect tail may follow.
- Preserve transport controls, menu navigation, and audio routing until Panic is invoked.

### Checks

- The host unit-test suite and a local Release build must pass.
- On a physical Deluge, invoke Panic in each reachable normal-mode scenario: playing Synth or Kit, Audio Clip, browser preview, MIDI or gate note, delay feedback, stutter, modulation effects, reverb, resonant filter, and compressor-driven audio. Confirm silence.
- When a later caller exposes Panic during recording or stem export, check that it aborts those captures too.
- Confirm project content, settings, automation, and undo history remain unchanged, apart from deliberately discarded unfinished capture.

## 10. Copy, paste, and reorder a Kit Sound Drum row

### Purpose

Duplicate a Sound Drum with or without its row notes, and undo accidental Kit-row moves.

### Behavior

#### Copy and paste

- In Kit Clip View, hold an assigned Sound Drum's Audition pad and press Learn to copy its instrument without changing the source.
- This first copy includes the sample or synthesis definition, sample bounds, loop settings, sound parameters, and MIDI mappings. It excludes ordinary row notes and playback defaults.
- Press Learn again within the normal short double-press interval, still holding the same source, to replace the copy with the instrument plus ordinary notes and row defaults, including mute, loop length, direction, probability, iterance, fill, and colour.
- Hold the target's Audition pad, then Shift + Learn, to paste. A full copy replaces its instrument, notes, and row defaults. An instrument-only copy replaces its Sound Drum and sound parameters while keeping the target's notes and row defaults.
- Order distinguishes paste from MIDI unlearn: Shift + Learn before holding an Audition target starts normal unlearn. Once MIDI Learn is active, copy and paste must not take its Learn input.
- An empty target creates a row. An existing Sound Drum target stops before replacement.
- Prepare all resources before changing the destination. Allocation failure leaves it unchanged.
- Source and destination remain independent: later edits to file references, sample markers, loops, sound parameters, notes, or row settings affect only the edited drum.
- Both must belong to the open Kit in the current song. The clipboard is neither saved nor shared across Kits or songs.
- MIDI and Gate drums cannot be copied or replaced. Report an occupied invalid target as unavailable without modifying it.
- Show clear copy-mode, copy-complete, paste-complete, allocation-failure, and unavailable-target feedback on OLED and seven-segment displays.

#### Reorder

- In Kit Clip View, hold the assigned source row's Audition pad, then the assigned destination row's Audition pad, and press Vertical. Move the source to the destination position and shift intervening rows. Both moved rows remain usable.
- A successful move creates one normal Undo action. Undo restores the exact preceding order; Redo repeats the move. Preserve earlier undo history.
- Move positions only. Each row keeps its instrument, notes, settings, automation, and identity.
- If the gesture does not identify two distinct assigned rows in the current Kit, change nothing and create no Undo action.

### Compatibility

- Audition + Horizontal still edits row length; held Horizontal still rotates a row. Learn + Horizontal retains its note and automation clipboard behavior and does not invoke this copy/paste feature.
- Copy no sound or row-parameter automation. A successful paste clears undo history; it does not offer partial undo.
- Preserve Kit master settings, other rows, source data, sample files, project saving, and MIDI playback.
- Keep the reorder gesture, direction, and row-shifting behavior. Do not replace it with drag-and-drop or extend it across Clips or Kits.
- Do not add persistent or cross-Kit copying, or MIDI/Gate cloning.

### Checks

- Review resource preparation and verify an allocation failure cannot partly change the destination.
- The host unit-test suite and a local Release build must pass.
- On a physical Deluge, check full copies to empty and assigned Sound Drum targets, exact marker and loop transfer, independent edits, full note transfer, instrument-only preservation of target notes, MIDI/Gate rejection, ordinary Horizontal controls, feedback, and save/reload.
- Check reorder, Undo, Redo, and an earlier unrelated Undo action.

## 11. Optional incoming MIDI sustain pedal

### Purpose

Let an external MIDI sustain pedal hold notes on an internal Deluge Synth, including DX7 sounds. Keep existing MIDI behavior unless the player enables the feature.

### Behavior

- Community Features contains a persisted `MIDI Sustain Pedal` setting that defaults to Off.
- With the setting Off, incoming CC64 and incoming note-offs retain their established behavior.
- With the setting On, CC64 values 64–127 sustain notes after the player releases their keys. This applies to a matched external MIDI input on an internal Synth track. Values 0–63 release only the notes whose keys have already been released.
- A note that remains physically held when the pedal is released continues sounding normally. If the player replays a pitch before releasing the pedal, a later pedal release must not stop the new held note.
- Apply the feature through the ordinary internal Synth input path. DX7 sounds get the same pedal behavior without a separate setting, sound-file field, or preset conversion.
- MIDI Learn handles CC64 before sustain handling. A player can still learn or unlearn CC64 normally, and a normal learned CC64 mapping may continue to receive its value.
- Turning the setting Off releases any notes already held by the pedal. Stopping or replacing the receiving Synth, and Panic, also clear the pedal-held state so it cannot leave a later note sounding.
- Handling pedal-held note-offs for all 128 MIDI pitches must not allocate memory during live events. Low memory must not cause an exception, reset, or stuck note. The same rule applies when disabling the feature or clearing a receiving Synth.
- Clip recording retains the physical note-off timing. This deviation does not record pedal events or change saved song, synth, or automation data.

### Compatibility

- Do not add half-pedal response, sostenuto, pedal recording, a new MIDI mapping format, or a global MIDI filter.
- Do not apply this behavior to Kit, Audio, CV, or external MIDI-output tracks. Sequenced notes, grid-pad audition, external MIDI output, existing sound envelopes, DX7 synthesis, and Community Feature settings other than this opt-in toggle retain their established behavior.

### Checks

- A local Release build and the configured host test suite must pass.
- On a physical Deluge with a CC64 controller, verify the setting defaults Off; pedal-down and pedal-up behavior for a single note, chord, repeated pitch, and a still-held pitch; DX7 behavior; normal MIDI Learn and unlearn; a learned CC64 mapping; target change; Stop; Panic; and save/reload.

## 12. Song Grid existing-track column glow

### Purpose

Show which Song Grid columns contain tracks before a clip is launched or selected. Preserve the existing clip-state colors and brightness.

### Behavior

- In the Grid layout of Song View, every visible main-grid column that represents an existing track has a neutral-white base in its otherwise empty cells.
- The base uses neutral white at brightness 7 on the 0–255 LED scale, much dimmer than ordinary clip colors.
- A column that does not represent an existing visible track remains black.
- An actual clip cell continues to render its existing color and brightness over the base. Inactive, active, playing, selected, armed, solo, record, MIDI-learn, and pulse feedback retain their existing appearance and precedence.
- Scrolling changes which track columns receive the base according to the existing visible Grid mapping.

### Compatibility

- Do not add a setting, animation, color theme, persistent state, or a new clip state.
- Do not change clip launch, selection, creation, duplication, deletion, scrolling, zooming, section behavior, track order, session data, MIDI, audio, or existing side-column controls.
- The base applies only to the Song Grid's main pads. Other session layouts and unrelated views retain their current appearance.

### Checks

- A local Release build and the configured host test suite must pass.
- On a physical Deluge, compare an empty existing-track column, an empty non-track column, inactive clip cells, active or playing clip cells, and a scrolled Grid. Confirm that the structural base is visible but subordinate to every established clip-state color and brightness.

## 13. Repeated Audio Looper recording safety

### Purpose

Let a player record repeatedly into the same Audio Clip in `Looper/FX` mode without emptying the Clip, corrupting the active recorder, or crashing on a later pass.

### Behavior

- An empty Audio Clip may use the existing first-take flow: Record then Play starts without the project clock; stopping the take sets the project tempo.
- After that Audio Clip contains a sample, another recording pass into the same `Looper/FX` Clip uses the established project clock. A populated Clip must never use that first-take flow again.
- Stopping a valid later pass replaces the Clip's prior sample with the completed recording. The Clip remains present in the same song location, retains its activation and `Looper/FX` monitoring role, and is immediately usable for playback or another recording pass.
- Keep the prior sample assigned until the replacement is ready to install. Cancelling a pass, capturing no samples, reaching the file-size limit, or rejecting the recorder before completion must leave the prior sample assigned and the Clip usable.
- A recorder may detach only itself from the Output that currently owns it. If a second recorder fails to attach, it must not detach, abort, or replace the recorder already attached to that Output.
- Detaching a recorder again is harmless, including after it has finished. Destroying an unattached or stale recorder must not change another recorder or its Output.

### Compatibility

- Preserve continuous microphone or line-input monitoring in `Looper/FX`, the existing first-take tempo calculation, normal Clip playback, Clip naming after a successful recording, and the established recording controls.
- Preserve ordinary Audio Clip cloning and overdub behavior outside `Looper/FX`, including non-Looper inputs, Arrangement recording, and recordings into a newly created Clip.
- Do not delete, move, deactivate, or recreate the destination Clip as part of a successful replacement. Do not clear other Clips, their samples or automation, unrelated project data, or prior undo history.
- This deviation does not change the separate cloned-overdub path, redesign Song Grid or Row looping, guarantee rollback after a late storage failure, add a recording mode, change audio routing or file formats, or repair unrelated held-Clip cancellation gestures.

### Checks

- Use focused host checks to cover empty-versus-populated first-take eligibility and exact-recorder Output detachment where the host harness can represent those objects. The complete configured host test suite and a local Release firmware build must pass.
- On a physical Deluge, create a new Audio Clip, select microphone or line input with Audio Output set to `Looper/FX`, and complete the first Record + Play + Play-stop take. Confirm that it establishes tempo and remains playable.
- In that same Clip, complete at least ten further Record + Play + Play-stop passes. After every pass, confirm that the Clip remains present, contains the latest completed take, continues monitoring as before, and can immediately begin the next pass without freezing or crashing.
- During separate later passes, cancel before completion and stop once without captured audio. Confirm that the previous completed take remains assigned and playable.

## 14. Song Grid and Audio Clip track color

### Purpose

Use one Audio track color for its Song Grid column and Audio Clip waveforms.

### Behavior

- Every Audio Clip attached to an Audio Output uses that track's Song Grid hue.
- Full-screen and single-row Audio Clip waveforms use a pastel version of that hue. Preserve the waveform shape and the brightness used to show amplitude.
- A newly created Audio Clip uses its track color on its first render. A new clip in an existing Grid column immediately matches that column, and multiple Audio Clips attached to one Audio Output always share one waveform hue.
- Changing an Audio track's color through the Song Grid updates every waveform attached to that track on its next render. Neither fine nor coarse Grid color changes may select the reserved unassigned value.
- The established Shift plus Vertical Encoder color gesture in Audio Clip View changes the shared Audio track color. The equivalent Shift plus held-clip gesture in Row Song View does the same and redraws every visible Audio Clip on that Output.
- An in-place overdub keeps the existing Output color. An overdub that creates a new Audio Output receives and then follows that new Grid column's color.
- The shared Output color survives save and reload through the established project format. Continue reading and writing legacy per-clip color data for compatibility. Use it only as a fallback while an Audio Clip is temporarily unattached from an Output.

### Compatibility

- Do not change Instrument Clip note colors, Kit-row colors, section colors, Arranger clip-instance colors, or the Song Grid's active, inactive, playing, selected, armed, solo, recording, MIDI-learn, and pulse brightness behavior.
- Do not change waveform shape, amplitude analysis, sample data, audio playback, recording, overdub routing, track order, clip placement, or undo history.
- A project whose Audio Output has no assigned color acquires one nonzero track color before its waveform is shown. The unassigned value must not briefly show a red waveform or cause the Grid to choose a different hue later.

### Checks

- Use focused host checks to cover positive and negative color stepping, hue-range wraparound, accelerated steps, and crossing the reserved unassigned value. The complete configured host test suite and a local Release firmware build must pass.
- On a physical Deluge, check a fresh Audio track, a second clip in the same Grid column, Grid fine and coarse color changes, both Audio color gestures, multiple visible Row clips sharing one Output, an in-place overdub, an overdub-created Output, save and reload, and an older project whose Audio Output color is unassigned.
- Confirm the waveform stays in the same hue family as its Grid column while remaining visibly pastel, and that every excluded color and brightness system remains unchanged.

## 15. Long-file waveform peak addressing

### Purpose

Keep waveform navigation and peak lookup correct across the recorder's supported file range. Signed 32-bit overflow must not hide the waveform or select unrelated sample, byte, or cluster data.

### Behavior

- Waveform peak collection accepts visible sample positions beyond `INT32_MAX` and derives non-negative byte and cluster positions without signed wrap, including positions around the 2 GiB and 4 GiB byte boundaries.
- For loaded samples, the recorded audio-data byte length determines the valid waveform end. A live recorder derives its valid byte length from the captured sample count and complete sample-frame width without overflowing the conversion.
- Each visible column retains a signed sample interval and an unsigned file interval until a narrower index is proven valid. Treat a request as unavailable or beyond the waveform if it overflows, precedes the sample, exceeds the valid audio range, or cannot address the sample's cluster collection. It must not index a negative or wrapped column, byte, or cluster.
- Zooming, scrolling, pinning a marker, and calculating the rightmost legal viewport remain stable throughout the supported sample length. Zero-length samples open at a valid minimum zoom instead of underflowing.
- Existing project fields for a saved sample-editor viewport remain signed 32-bit values. Save a viewport only when both values fit and zoom is positive. Otherwise, save the existing unset values so reopening shows the full view rather than a wrapped or invisible one.

### Compatibility

- Preserve waveform shape, extrema, short-file navigation, marker pinning, sample playback, recording, project format, and the established behavior of representable saved viewports.
- Do not expand the supported media format, recorder size limit, cluster container, or persistent viewport fields as part of this deviation.
- Do not allocate memory, load extra clusters, or add background work merely to handle wide positions. When sample data is unavailable, the existing renderer must still be able to retry.
- Reversed row mapping is governed separately by deviation 16. OLED presentation is governed separately by deviation 17.

### Checks

- Use focused host checks to cover signed addition with negative, zero-crossing, maximum, and overflowing positions; byte positions immediately below and at 2 GiB and 4 GiB; sample positions above `INT32_MAX`; invalid frame widths; byte multiplication overflow; and invalid cluster-size magnitudes.
- Review the source to confirm that loaded and live-recorded lengths take their respective checked paths, all narrowing occurs after explicit bounds checks, and a final-cluster byte limit is calculated from the full valid audio byte position.
- Compile the waveform renderer, basic navigator, and sample-marker editor with the target ARM Release toolchain, then complete the configured host suite and a local Release firmware build.

## 16. Reversed half-open waveform row ranges

### Purpose

Mirror the requested interval when drawing a reversed single-row waveform. Do not produce a negative source column, drop an edge column, or read outside the fixed peak arrays.

### Behavior

- Treat every row request as a half-open interval `[start, end)`. Mirroring within a display of width `W` maps it to `[W - end, W - start)`.
- Reversing the full 16-column pad interval therefore remains `[0, 16)`. Reversing either one-column edge interval stays one column wide and inside the display. Partial intervals retain their width and mirror to the opposite side.
- Reject a peak-collection request before touching cached column state when its start is negative, its end precedes its start, or its end exceeds the display width.
- After a valid reversed lookup, each output column reads its mirrored source peaks. Preserve existing amplitude and color treatment.

### Compatibility

- Preserve all forward waveform rendering, peak analysis, sample and recorder access, display width, Audio Clip color, collapse animation, and row ownership.
- Do not clamp or silently reshape an invalid caller range. Return the renderer's existing incomplete result so the caller cannot mistake invalid cached data for a finished waveform.
- This contract applies to the generic single-row pad renderer. OLED marker and contour presentation remains a separate deviation.

### Checks

- Use focused host checks to cover the full display, both one-column edges, and several partial half-open intervals, asserting width preservation and in-bounds mirrored endpoints.
- Review the source to confirm that range validation precedes every `colStatus`, minimum, or maximum array access and that reversed output columns use the matching mirrored source column.
- Compile the affected waveform renderer with the target ARM Release toolchain, run the focused host checks together with the long-file regression, and complete the configured host suite and local Release build.

## 17. OLED sample-editing waveform companions

### Purpose

Show the sample waveform and edited region on the OLED. Do not add sample analysis, storage access, audio work, background animation, or repeated display transfers while the view is unchanged.

### Behavior

- `SampleMarkerEditor` shows a monochrome outline of the currently visible sample region beneath a compact marker header. This applies to instrument Sample start, end, loop-start, and loop-end editing and to Audio Clip start and end editing.
- The selected sample marker is a clear full-height bound. Other relevant start, loop, and end markers remain visible as smaller ticks without covering the waveform.
- Horizontal scrolling and zooming update the OLED waveform and marker positions only after the visible region changes. Reversed samples preserve the same left-to-right relationship as the pad waveform.
- Both Region and Manual/Lazy Slicer modes use a full OLED page, not a small overlay on Sample Browser.
- Region mode shows a compact region-count header, the sample outline, and equal-region divisions only when they remain far enough apart to read as distinct marks.
- Manual/Lazy mode shows the selected slice number, slice count, and current start value with the sample outline. All slice starts remain visible as small ticks, while the selected slice start and end are clear full-height bounds.
- Adjusting a Manual/Lazy start keeps the waveform visible; temporary text feedback must not cover the waveform. Redraw the OLED when slice selection, creation, deletion, count, mode, boundary, or a successful viewport change alters what is displayed.
- By default, measure 128 independent peak ranges, one per physical OLED column. This gives eight times the horizontal resolution of the 16 pads. A build-time setting may select 16, 32, 64, or 128 equal display ranges. Only waveform detail and analysis cost may change.
- Adjacent OLED ranges within one pad column must cover exactly that pad column's half-open sample interval. This also applies when the visible sample count does not divide evenly into display ranges. When there are enough visible samples for independent ranges, measure each sample in the range where it appears horizontally. At tighter zooms, adjacent ranges share samples rather than imply nonexistent detail. Ranges covering the same half-open sample interval reuse one peak result.
- If cached amplitude is unavailable for part of the view, keep the measured parts visible and leave the rest blank. The normal editing redraw can retry the missing parts. OLED rendering must not initiate sample analysis, decoding, cluster loading, storage access, memory allocation, or audio-engine servicing to fill it.
- Apply deviations 15 and 16: valid long recordings remain visible when zoomed out, and reversed views stay within the pad waveform's visible half-open range.
- A static editor schedules no waveform-driven display refreshes after its current state has been drawn. Except for the source-matched playhead required by deviation 25, playback, audition, marker blinking, and periodic graphics routines do not animate or refresh the OLED waveform.

### Compatibility

- Preserve the pad waveform's viewport, layout, colors, markers, and navigation, plus marker limits, slice calculations, playback, audition, saving, audio processing, control gestures, and project format. On these two OLED editing surfaces, each pad column may show more precise extrema from the same visible region. Do not analyze the samples a second time to obtain them.
- Preserve all 7SEG behavior. Do not add an OLED waveform to Sample Browser, idle Audio Clip View, Session, Arranger, recording screens, or other unrelated views.
- Except for the bounded source-matched cursor in deviation 25, do not add a playback cursor, stereo lanes, filled amplitude bars, inverted selection, a project-persistent or Audio-Clip-wide waveform cache, a timer, or a background waveform task.
- OLED waveform rendering must never run from an audio-rendering path. A physical power comparison is required before claiming a battery-life improvement.

### Checks

- Use focused host checks to cover the supported display-range counts, exact viewport and pad-column boundary equivalence at remainder-heavy zooms, intentional shared ownership at tight zooms, one-column default range placement, silence, full-scale and asymmetric peaks, reversed views, partial availability and retry, beyond-waveform columns, first and last columns, vertical amplitude bounds, and deterministic fixed work for the cached input width. The generic long-address and reversed-range checks remain owned by deviations 15 and 16.
- Review that OLED rendering uses only available peak data, allocates no memory, performs no sample or storage work, and adds no graphics or playhead timer. Check that it requests a redraw only after a relevant editor state changes.
- Compare the baseline and changed Release ELF sizes, run the complete configured host test suite, and complete a local Release firmware build.
- On a physical OLED Deluge, check instrument Sample start, end, loop-start, and loop-end; Audio Clip start and end; zoomed, scrolled, and reversed views; Slicer Region divisions; and Manual/Lazy slice selection and boundary movement. Leave each editor idle while audio, storage streaming, CV output, and optional OLED mirroring are active, and confirm the waveform remains readable without visible display churn or new audio interruption.

## 18. OLED Clip timeline ruler

### Purpose

Show progress through the whole Clip loop, beat marks, and the range visible on the pads. Keep the ruler compact enough to preserve existing sound, Clip, and recording information.

### Behavior

- Show a monochrome ruler in the three display rows at the OLED's top visible edge. It appears in normal Instrument Clip View for Synth, Kit, MIDI Out, and CV Clips; normal Audio Clip View; every non-Arranger Clip Automation surface; and every Keyboard View layout. Supported Automation surfaces include Automation Overview, parameter automation, and note Velocity editing for Instrument and Audio Clips where those modes apply. Supported Keyboard layouts are Isomorphic, In Key, Piano, Chord, Chord Library, Velocity Drums, and Norns.
- The ruler's 128 columns always span the whole Clip loop from zero to its exclusive end. Horizontal scroll, zoom, and Triplet layout changes must not rescale the ruler. The playhead must not reset when it leaves the 16-pad view.
- In timeline Clip and Automation views, a separate clipped span shows the part of the Clip visible on the 16 main pad columns. Horizontal scroll moves that span, zoom changes its width, and a view covering the complete Clip fills the ruler. A pad view entirely outside the Clip shows no false selection. Keyboard View never shows this span because its pad columns represent playable notes, chords, or drums rather than Clip time.
- The top row shows played progress and the bottom row shows musical marks. In timeline Clip and Automation views, the middle row shows the visible pad range; in Keyboard View it remains empty except where the Clip caps or live playhead cross it. Quarter-note marks are one pixel wide. Bar marks are two adjacent pixels wide; a bar at the right edge shifts inward instead of being clipped to one pixel. Clip start and end caps and the live playhead cross all three rows and are drawn over those lanes.
- Quarter-note marks appear when they remain at least four pixels apart across the complete loop. At greater density the ruler uses bar marks and then evenly coarser bar intervals. Marks remain bounded and never merge into a solid block.
- While the current Clip is actively playing or recording, a three-pixel playhead crosses the ruler and a one-pixel progress segment reaches from Clip time zero to that position. The indicator follows forward, Reverse, and Ping-Pong motion, continues beyond the visible pad range, traverses the OLED once per complete loop, and wraps only at the Clip boundary.
- A stopped Clip keeps its start/end caps, musical marks, and visible-range selection, without moving progress. Count-in and an inactive Clip likewise show no false live position.
- A cloned overdub shown against its source Clip follows the same repeated-position and direction behavior as the established pad playhead.
- During clocked linear Arrangement Audio recording, use the length recorded so far as the provisional whole-Clip length, not the maximum-length sentinel. Keep the playhead at the growing right edge. A tempoless first-loop recording has no settled musical length, so it shows an explicit full-width progress and right-edge playhead state without beat marks or a false viewport selection until the musical length is established.
- Kit Clips with independently looping rows show the master Clip timeline. The single OLED ruler must not imply that all independently looping rows share another row's position.
- Normal OLED notifications and popups take visual precedence over the ruler. Entering Performance View, Arranger Automation, a menu, Sound Editor, Song or Arranger View, Audio Recorder, Sample Browser, Slicer, Sample Marker Editor, stem export, or a view transition removes or suppresses the live ruler update.
- Static ruler changes appear immediately after Clip change, scroll, zoom, transport state change, or recording-state change. Update moving playback and growing recordings no more than 20 times per second, and only when the visible result changes. Unrelated full OLED redraws between updates must not bypass this limit.

### Compatibility

- Preserve every existing Clip title, parameter value, icon, popup, side scroller, stem-export display, pad playhead, pad color, navigation gesture, Clip timing rule, recording rule, and project format. In Keyboard View, preserve every layout, playable-pad behavior, recording tick, chord name, latest physical note, layout feedback, and temporary popup.
- Preserve 7SEG behavior and every OLED surface outside normal Instrument and Audio Clip views, non-Arranger Clip Automation, and Keyboard View.
- The ruler is display-only. It must not add a separate timer, allocate memory while updating, read samples or storage, scan a Clip's events, run from an audio-rendering path, send an unchanged display frame, or increase per-Clip saved or runtime state.
- Do not add a Song-level transport ruler, waveform, per-row Kit ruler, playhead trail, animation outside active playback or recording, user setting, or color option.

### Checks

- Use focused host checks to cover complete-loop mapping; ordinary, scrolled, zoomed, clipped, and Triplet-derived viewport ranges; quarter, two-pixel bar, right-edge bar, and coarsened marks; maximum supported timeline positions; forward, Reverse, Ping-Pong, wrapped, stopped, playing, and recording positions beyond the pad view; cloned overdubs; clocked Arrangement growth; tempoless first-loop state; exact three-row bounds and lane separation; unchanged-state suppression; full-render cache reuse; and the 20 Hz moving-update ceiling.
- Review the source to confirm that normal Instrument and Audio Clip views, non-Arranger Clip Automation, and Keyboard View own the ruler; Automation and Keyboard repaint it after replacing the OLED canvas; Keyboard uses no timeline viewport selection; inactive delegated Clip views do not invalidate another supported surface's shared cached frame; existing musical and viewport sources remain authoritative; all mapping work is fixed and bounded; and OLED animation adds no timer, allocation, storage or sample access, audio-path work, direct display transfer, or unchanged redraw.
- Compare baseline and changed Release ELF sizes, run the complete configured host test suite, and complete a local Release firmware build.
- On a physical OLED Deluge, check Synth, Kit, MIDI Out, CV, and Audio Clips in their normal Clip views and in non-Arranger Automation Overview, parameter automation, and note Velocity editing while stopped, playing, recording, wrapping, reversing, zooming, and scrolling.
- Check all seven Keyboard View layouts while stopped, playing, recording, counting in, switching layouts, showing chord and latest-note feedback, opening and closing a menu, and returning to Instrument Clip View during playback.
- Confirm Keyboard has no viewport-selection span, Performance View and Arranger Automation remain unchanged, titles and popups remain clear, movement is readable without dominating the screen, and simultaneous CV output, audio playback, recording, and storage streaming show no new interruption.

## 19. Waveform Editor exact Kit-row audition

### Purpose

Let Select audition the Kit Sound Drum being edited, without leaving Waveform Editor or finding its pad.

### Behavior

- In a Kit Sound Drum's Waveform Editor, pressing and holding the Select Encoder auditions the Sound Drum being edited. Releasing Select ends the held audition using the Sound Drum's normal playback behavior. Modes that track a held note receive a matching note-off. Once and other no-tail modes finish as they normally would.
- The audition uses the Sound Drum's current sample Start, End, loop bounds, reverse, transpose, and Cut, Once, or Loop playback settings. The waveform's visible scroll and zoom range do not become playback bounds.
- The audition remains attached to the Sound Drum that started it. Vertical Kit scrolling, waveform navigation, marker editing, or a later change to the selected Kit row must not redirect its release to another row.
- The preview bypasses the Clip-level Kit arpeggiator so it cannot be treated as a numbered Clip row. It uses the Sound Drum's normal note path, including its own arpeggiator, sample playback, effects, and the parameter state of the row that began the preview.
- A Kit active-Clip change ends and resets the dedicated preview before the new Clip becomes active, including Once and other no-tail playback. Releasing the earlier Select hold afterward is harmless and must not affect the new Clip.
- Select does not start or layer a dedicated preview while that same Sound Drum still has any active voice, including a Once or Cut voice playing to completion and an envelope release tail. Once the Sound Drum has become silent, Select can start the preview normally. This ensures that a hard stop on an active-Clip change cuts only voices created by the preview.
- If the original Clip's rows are reordered or its preview row disappears while that Clip remains active, release still targets the original Sound Drum and uses only that Clip's row parameter state. It must not release the row now occupying the old position, leave the Sound Drum's own arpeggiator input held, or borrow parameter state from another Clip. If the original parameter state no longer exists, the preview is stopped and reset rather than released through unrelated state.
- While Select is held, protect the preview from other choke-group activity. Remove that protection immediately when the preview ends, is replaced, or is hard-cancelled.
- Select Encoder rotation retains its existing marker-editing behavior while Select is held.
- Pressing a normal row Audition pad while the dedicated preview is sounding stops the dedicated preview before the row Audition action begins. If a normal row audition is already active, Select must not start a competing preview.
- A later sequenced note, normal row audition, or MIDI audition of the same Sound Drum replaces the dedicated preview before the new note begins. Releasing the earlier Select hold after that replacement is harmless and must not stop the newer note.
- The preview follows the Kit's actual active Clip. If the edited Clip cannot become active, Select may audition the same Sound Drum through another active Clip only when that active Clip contains a row assigned to it. Do not start the dedicated preview while the project clock is active and the active row is sequenced. The preview must not override sequenced playback.
- Leaving Waveform Editor, including through Back, stops any dedicated preview. Repeated release or exit handling is harmless and must not leave the preview marked as active.
- Panic or another external all-stop invalidates the dedicated preview. Releasing the earlier Select hold afterward must not send a stale note-off that cuts a later MIDI or row retrigger of the same Sound Drum.
- If storage work defers Select, wait until the press is handled before starting the preview. A deferred release must still stop it.

### Compatibility

- Preserve normal row Audition pads, Sound Drum selection, marker movement, waveform scroll and zoom, recording, MIDI input, saved project data, and song transport.
- Do not use the visible Kit row position to identify the preview target and do not record a note merely because the dedicated preview is used.
- This action applies only to a Kit Sound Drum's Waveform Editor. Sampled Synth and Audio Clip Waveform Editors retain their established playback controls, and modified Select gestures retain their existing behavior.
- The preview must preserve the Sound Drum's own arpeggiator, choke, effects, sample bounds, and playback-mode behavior. Only the Clip-level Kit arpeggiator is bypassed; unrelated sequenced voices must not be cut.

### Checks

- Compile the affected Waveform Editor source with the target ARM Release toolchain, run the configured formatting checks and host test suite, and complete a local Release firmware build.
- On a physical Deluge, enter Waveform Editor from a sampled Kit row, scroll the Kit so that row is no longer at its former screen position, and confirm that Select still auditions and releases the edited Sound Drum.
- Check marker changes while holding Select, Start and End bounds, reverse, transpose, Cut, Once, and Loop, the Sound Drum's own arpeggiator, choke groups and effects, a pre-existing row audition, switching to a normal row Audition pad, repeated Select presses and releases, Back while sounding, and a storage-busy deferred press and release.
- Start the same Sound Drum normally in Once and Cut modes, and during an audible release tail, then press Select: no dedicated preview may layer over the existing sound.
- After the Drum becomes silent, Select must preview it normally.
- While the dedicated preview is held, trigger another choke-group Sound Drum and confirm the preview remains protected; after releasing or replacing it, confirm normal choke behavior resumes.
- With the clock running and the active row sequenced, confirm the dedicated preview does not start or override sequenced playback.
- When the edited Clip cannot become active, confirm the same Sound Drum is previewed only if the actual active Clip owns a row for it.
- Schedule another Clip on the same Kit to launch while Select is held, including Cut, Once, and Loop previews and a destination Clip where that Sound Drum occupies another row index and plays a note.
- The preview must stop before the active Clip changes, the destination note must start normally, and the later Select release must not target or cut it.
- Repeat after reordering the original Clip's rows, and remove the original preview row while keeping the Clip active to verify exact-Clip release or the hard-stop fallback without borrowing another Clip's parameters.
- While Select is held, retrigger the same Sound Drum from a row pad and from MIDI; each new audition must replace the dedicated preview, and the later Select release must not cut it.
- After Panic, retrigger the same row from MIDI before releasing Select and confirm the stale release does not cut that retrigger.
- Confirm that no note is recorded and no preview remains stuck.
- Confirm that sampled Synth and Audio Clip Waveform Editors, modified Select gestures, row audition, recording, and transport remain unchanged.

## 20. Recoverable Audio Clip Start editing

### Purpose

Let a player trim an Audio Clip's playback Start, then recover earlier source audio without Undo or leaving Audio Clip View.

### Behavior

- When the existing `Trim From Start Of Audio Clips` Community Feature is On, Audio Clip View shows the playback Start as a faint green boundary and End as a faint red boundary. Material before playback Start is visibly dimmer than material inside the Clip.
- If the source file contains audio before playback Start, ordinary Horizontal scrolling can reveal that material as negative Clip-time editing space. The space contains only real source audio and ends exactly at the recoverable source boundary. It is not silence and does not play until Start is moved into it.
- Every other timeline surface retains a zero-or-later minimum scroll position. Entering Session, another Clip type, or another view from negative Audio Clip space restores a legal position for that surface.
- Tapping the green Start or red End pad selects that playback boundary. The selected boundary blinks in its own color. Turning Select without Shift moves only the selected boundary at the current horizontal zoom resolution.
- If Start and End occupy the same pad column, the shared boundary is shown as a dim yellow marker when neither is selected. Pressing that column selects Start first and then alternates Start and End. If there is a column immediately to its right, pressing it selects End directly.
- With no boundary selected, Select retains Audio Output mode selection. Shift plus Select retains Audio Output assignment. Ordinary Horizontal rotation always scrolls and pressed-Horizontal rotation always zooms, even while a boundary is selected.
- Clockwise moves a boundary later in playback time; counterclockwise moves it earlier. Forward and reversed Clips behave the same to the player, even though reversing swaps which raw file boundary represents playback Start and End.
- Moving either boundary changes musical length using a fixed source-to-tick ratio. Calculate every detent from the values at the start of the encoder gesture so rounding error does not accumulate.
- Start can expand only into recoverable source audio and can trim only until one source sample and one musical tick remain. End retains its established ability to trim or expand into source material. Neither boundary can cross the other or exceed the maximum supported Clip length.
- A fast turn reaching a raw source edge or sequence-length limit stops exactly at that limit. The first reverse detent moves back inward, without requiring the player to unwind acceleration overshoot.
- Moving Start keeps the visible source position anchored as the Clip rebases to musical tick zero. Moving End retains its established visible-space behavior.
- One uninterrupted run of Select detents on one boundary is one Undo action. Undo and Redo exchange the exact raw boundary and musical Clip length together. Switching boundaries, using another control, changing Clip or view, or otherwise ending the gesture closes that action.
- Reject a boundary edit before changing either value if recording or count-in is active, storage owns the data path, Undo history cannot be allocated, the Clip or source state is invalid, or the result would be illegal. A rejected edit neither creates an empty Undo action nor destroys existing Redo history.
- During ordinary playback, accepted edits keep the raw boundary, Clip length, automation, Arrangement instances, and resumed playback consistent with one another. Repeated Undo and Redo must not destructively trim automation before its saved state is restored.
- When `Trim From Start Of Audio Clips` is Off, Audio Clip View does not expose negative pre-Start space or a Start-selection affordance. Existing End selection, relocation, scrolling, zooming, and output controls remain available without allowing a Start-pad press to collapse the Clip.

### Compatibility

- Preserve the audio file, sample data, recording, monitoring, launch behavior, time-stretch choice, Output assignment, Output mode selection, existing End relocation gesture, Clip color, waveform analysis, and project format.
- Do not add stored negative time, prepend silence, modify Waveform Editor, or expose pre-Start space in Instrument, Kit, MIDI, CV, Automation, Session, Song, or Arranger views.
- Marker colors and directions are playback-relative. Reversal must not expose the wrong source edge or swap the meaning presented to the player.

### Checks

- Use focused host checks to cover forward and reversed Start and End movement, exact raw-edge saturation, sequence-length saturation, minimum length, wide source positions, half-tick rounding, cumulative gesture ratios, fast-boundary reversal, source-backed negative-scroll limits, and signed scroll and zoom navigation.
- Review the source to confirm explicit Start or End selection, modifier ownership, feature-Off compatibility, coincident-marker selection, gesture-closing boundaries, recording and storage guards, allocation-safe history creation, exact coupled Undo and Redo, automation consequence ordering, and non-Audio scroll clamping.
- Compile every affected Audio Clip, timeline, waveform, action-history, and consequence source with the target ARM Release toolchain; run the complete configured host suite and a local Release firmware build.
- On a physical Deluge, check forward and reversed Clips; source-backed scrolling; Start and End trim and recovery; fast turns into each edge followed by immediate reversal; one-tick and same-column markers; Shift and both encoders; active playback; normal recording, count-in, and storage-busy rejection; repeated Undo and Redo with automation; feature On and Off; and navigation into Session and other Clip types.

## 21. Explicit Waveform Editor entry and bound selection

### Purpose

Show sample information before opening the graphical Waveform Editor. Return to that information screen without losing the selected boundary.

### Behavior

- Every supported Waveform, Sample Start, and Sample End launcher for Audio Clips, sampled Synths, and Kit Sound Drums opens the existing sample-information or sample-settings screen first, without drawing the graph.
- The relevant Waveform or boundary item is focused on that information screen. Pressing Select without Shift opens the graph. Pressing Shift plus Select on the information screen does not open it.
- Back from the graphical editor returns to the exact parent information screen and focused item. A subsequent Back follows the established menu path.
- The graph renders every active sample boundary that falls inside the visible source range. Sample Start and Sample End remain visible together; loop boundaries remain governed by their established availability. The controlled boundary is visually dominant and the others remain distinguishable.
- Sample Start is the initial controlled playback boundary. Leaving with Back and reopening the same sample preserves the chosen Start or End boundary. Editing a different sample resets the selection predictably to Start.
- One Shift plus Select press in the graph switches the controlled playback boundary between Start and End. Switch once on the press and consume its release, even if Shift is released first. It never selects a loop boundary.
- Boundary names, colors, and switching remain playback-relative for reversed samples.
- Turning Select retains the established movement rules for the controlled boundary. Ordinary Horizontal rotation scrolls and pressed-Horizontal rotation zooms. Marker-pad selection and loop-boundary behavior remain available.
- In a Kit Sound Drum's Waveform Editor, unmodified held Select retains its dedicated momentary audition behavior. Shift plus Select changes the controlled boundary without starting, stopping, or latching that audition.
- Keep existing refusal or deferral behavior for invalid or missing samples and busy storage. A rejected entry or switch leaves no stale selection and cannot consume a later unrelated Select press.
- OLED and seven-segment displays both identify the controlled boundary using their established display capabilities. Returning to information and reopening must not change sample data, bounds, playback, or the visible waveform range.

### Compatibility

- Preserve raw sample-bound semantics, loop-point creation and removal, marker movement limits, waveform resolution and caching, scrolling, zooming, reverse playback, transpose, playback mode, audition, recording, Kit row ownership, Slicer behavior, project data, and audio processing.
- This contract does not change Audio Clip View, its Clip-time Start and End controls, negative pre-Start navigation, or Clip musical length. Those belong to deviation 20.
- Do not create a second information screen, merge the two editors, force offscreen bounds into view, add persistent edge indicators, or change sampled Synth and Kit playback behavior.

### Checks

- Review every supported launcher and parent menu, Audio Clip and sampled Synth or Kit context, multi-range samples, reverse playback, same-sample re-entry, different-sample reset, storage deferral, modifier release order, loop-boundary preservation, and Kit audition ownership.
- Compile every affected menu and graphical editor source with the target ARM Release toolchain, run formatting checks, run the complete configured host suite, and complete a local Release firmware build.
- On a physical OLED and seven-segment Deluge, check each launcher, information-to-graph Select, graph-to-information Back, the next Back, both visible bounds, Shift plus Select in both release orders, Start and End movement, loop markers, forward and reverse playback, same-sample re-entry, a different sample, Kit held-Select audition, and storage-busy behavior.

## 22. Velocity Drums velocity profiles

### Purpose

Let players choose the number of velocity targets in each large Velocity Drums block. Keep the existing layout and drum placement.

### Behavior

- Velocity Drums offers four profiles named `FULL`, `4`, `2`, and `FIXED`. Save the profile and fixed velocity separately for each Clip.
- `FULL` is the default for new Clips and for projects that do not contain valid profile data. It preserves the established velocity and brightness of every cell exactly.
- In a four-by-four drum block, `4` divides the pad into four two-by-two quadrants. The bottom-left quadrant emits velocity 32, bottom-right emits 64, top-left emits 96, and top-right emits 127. Every cell in one quadrant has the same brightness and emitted velocity.
- `2` divides a multi-row drum block into lower and upper halves at velocities 64 and 127. A one-row block instead uses left and right halves at those velocities.
- `FIXED` makes every cell in the drum block emit the saved fixed velocity. Limit it to 1 through 127; use 64 when saved data is missing or invalid.
- If a block has too few cells for the selected profile, divide its available horizontal or vertical cells into no more than the requested number of regions, ordered from soft to loud. A one-cell block uses the saved fixed velocity.
- For `4`, `2`, and `FIXED`, each cell's brightness represents the velocity that the same cell will emit. Disabled drum blocks remain unlit, and an active note retains the established active-note dimming.
- In a Kit Clip using Velocity Drums, holding Scale and turning Horizontal selects the profile. Holding Scale and turning Vertical changes the fixed velocity and selects `FIXED`. Accelerated encoder offsets remain bounded or wrap only within the four profiles.
- Pressing and releasing Scale without an encoder turn reports the current profile instead of showing the Kit scale refusal. The matching release remains consumed even if the Output or Keyboard layout changes while Scale is held.
- A Load chord must not change or report the velocity profile. Scale-modified encoder actions take precedence over Shift color editing, pressed-Horizontal zoom, and Shift zoom only while Velocity Drums owns the Scale press.
- Profile and fixed-velocity changes affect only later pad presses. They must not retrigger, stop, rewrite, or change the velocity of sounding or recorded notes.
- Profile and fixed-velocity fields load independently. An invalid field falls back to its own default without resetting the other field or the Clip's existing Velocity Drums scroll and zoom state.

### Compatibility

- Preserve drum order, block geometry, scroll, zoom, selected-drum tracking, retrigger choice, recording, note-off behavior, Clip color editing, and the complete legacy `FULL` velocity and brightness behavior.
- Preserve the established Scale button behavior in every non-Kit surface and every Kit Keyboard layout other than Velocity Drums. Leaving and returning to Keyboard View must not leave a stale Scale gesture.
- Do not repair the pre-existing widened final block mismatch at the three-cell horizontal zoom or the pre-existing zero-velocity edge at the largest `FULL` zoom as part of this deviation.
- Do not add a duplicate Keyboard layout, a timer, background work, persistent Song-wide state, Kit-row sample state, sequencer note editing, MIDI velocity processing, or the separate proposed note-length gesture.

### Checks

- Test independent saved-field validation, fixed-value clamping, exact `FULL` passthrough, four-by-four quadrant orientation, two-half orientation, fixed velocity, one-cell and single-axis blocks, odd and wide geometries, nonzero new-profile velocities, and brightness mapping.
- Verify that `FULL` keeps its legacy render path; Scale ownership, Load suppression, and encoder precedence cannot leak to another layout; profile changes do not touch active notes; and saving and loading use the existing per-Clip Keyboard state without disturbing scroll or zoom.
- Format every affected source, compile the changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete a local Release firmware build.
- On a physical Deluge, check every profile and zoom, four-quadrant orientation, two-region orientation, fixed-value limits, accelerated turns, OLED and seven-segment feedback, held notes, retriggering, scroll, zoom, color editing, Load chords, layout changes while Scale is held, and save and reload.

## 23. Externally stepped MIDI Clips

### Purpose

Let MIDI Clips run as independent step sequencers for modular software such as VCV Rack. Each Clip advances through learned external Step and Reset controls rather than the Song tempo.

### Behavior

#### Controls and priority

- Community Features contains an External Step MIDI Clips setting that defaults to Off.
- Session MIDI-Out Clips have a Clock setting with Song and External Step modes. External Step is a MIDI Clip clock mode, not a new Output type.
- Keep existing and new MIDI Clips in Song mode unless the player selects External Step while the Community Feature is On.
- A saved External Step Clip keeps its Clock settings accessible when the Community Feature is Off, so the player can select Song mode. The Clip remains stopped until then and never falls back to Song timing.
- External Step provides separate learned Step and Reset controls. Match each control by its exact connected MIDI input, raw physical MIDI channel, message type, and number. Raw channel matching remains exact when the input port is configured for MPE; MPE zones do not widen or translate the binding. Supported message types are Note, Control Change, and Program Change.
- A valid Step control is required before the Clip can advance. Reset is optional. Evaluate it independently of Step assignment, connection, conflict, and Clip timing eligibility. An unassigned, missing, invalid, or conflicting Reset disables only Reset and does not block an otherwise eligible Step. A valid nonduplicate Reset remains usable when Step is invalid.
- For a Note control, one positive-velocity Note On creates an edge and later Note On messages do nothing until Note Off or a velocity-zero Note On rearms it. For a Control Change control, crossing from values 0 through 63 into 64 through 127 creates an edge and later high values do nothing until a value below 64 rearms it. Every matching Program Change creates one edge.
- A Clip cannot assign the same exact control to both Step and Reset. The same Step or Reset control may be shared by several Clips. One shared Step edge advances each eligible matching Clip once, and one shared Reset edge resets each active matching External Step Clip with a valid Reset once, even when that Clip's Step is invalid.
- When one message is Reset for some Clips and Step for others, every matching Reset occurs before any matching Step.
- Active MIDI Learn always receives a prospective binding first and learning a control never invokes it. Existing global, section, Clip, Kit-row, and parameter mappings retain priority. A conflicting External Step control remains saved but inactive. A Step conflict makes Step unavailable; an optional Reset conflict disables and reports Conflict only for Reset while Step remains governed by its own status.
- Whenever a higher-priority mapping changes a Step or Reset binding into or out of Conflict, that binding's held Note or Control Change state rearms. Program Change has no held state and remains one edge per matching message. A newly conflicting Step cuts the Clip's owned notes once while preserving phase; repeated conflict checks do not repeat the cut.
- While the feature owns a valid active binding, its matching Note, Control Change, or Program Change messages do not audition a sound, record notes or automation, enter MIDI Follow, control an Output, or pass through MIDI Thru.

#### Playback and cleanup

- Global Play enables External Step advancement but the Song clock never moves an External Step Clip. Launching or relaunching the Clip places it before step zero and shows that it is waiting.
- The first accepted Step edge after launch, Stop, or Reset emits position-zero events once, without advancing first. Each later accepted Step edge advances by exactly the selected step.
- Reset works with transport running or stopped, whether or not Step is valid. It cuts notes owned by the Clip, returns the Clip to the position immediately before step zero, clears cadence state, and emits no step-boundary note-on or automation. Reset does not rearm a held Note or Control Change; the control still needs its release or low value. The next accepted running Step emits position zero.
- Clip stop, mute, replacement, MIDI-Out reassignment, and feature disable cut notes owned by the Clip, rearm its Note and Control Change controls, and return it to launch state. Global Stop and leaving Session apply that cleanup to every configured External Step Clip, active or inactive. Cleanup emits no step-boundary note-on or automation; a note-off needed to cut an owned note is the only permitted MIDI output.
- Panic cuts owned notes and clock-loss state while preserving the current phase and held-edge state. The next Step after Panic continues from that frozen phase unless the Clip is also stopped or reset.
- Supported step sizes are one quarter, one eighth, one sixteenth, and one thirty-second note. Save the size independently of pad zoom. Use one sixteenth when it is absent or invalid.
- Support forward Session MIDI-Out Clips whose master loop length is evenly divisible by the selected step. Changing Clip direction to Reverse or Ping-Pong, or giving a NoteRow independent length or direction, immediately cuts owned notes, rearms Step and Reset Note or Control Change state, and returns the Clip to pre-step-zero. The Clip then remains visibly unsupported and inactive. Restoring supported timing also remains at pre-step-zero and cannot sound through ordinary Song resume behavior. This cleanup must not move, quantize, or rewrite stored notes. Only the requested timing edit may change them, using its existing behavior.
- Notes and stepped MIDI Control Change automation execute only on pulse boundaries. An off-grid event waits until the first boundary at or after its stored position and remains unmodified in the project. A note shorter than one pulse lasts one pulse.
- All due note-offs occur before any note-ons at a boundary. When several starts in one NoteRow collapse onto one boundary, only the latest stored start before that boundary emits; the earlier starts remain saved.
- Probability and iterance retain their established decisions at the emitted boundary. Smooth automation, swing between pulses, arpeggiators, MPE timing, and other sub-pulse behavior are not part of this mode.
- FIRST iterance follows the externally stepped Clip's own repeat count. LAST iterance does not fire in External Step mode because incoming pulses have no defined final iteration; Song-clock stop scheduling cannot supply one. Ordinary Song-clocked Clips retain their normal FIRST and LAST behavior.
- A stable-clock watchdog qualifies only after three consecutive intervals from 25 milliseconds through 4 seconds remain within 25 percent of their median. Four missed median intervals cut owned notes and show Waiting without changing phase. Slower or irregular controls continue stepping but do not establish automatic clock-loss timing.
- The next Step after qualified clock loss continues from the frozen phase. A disconnected input clears held-edge state and cuts owned notes only when that exact input has no remaining connection.

#### Saved state

- Clip mode, step size, Step binding, and Reset binding survive save, load, and Clip duplication. Do not save or copy held-edge, phase, cadence, or pending note-off state.
- Missing or invalid fields recover independently. Missing mode means Song, missing size means one sixteenth, invalid Step keeps External Step visibly inactive, and invalid or missing Reset remains unassigned.
- Assigning or loading an internal Synth, Kit, or CV output changes the Clip's Clock mode to Song before normal playback. A valid MIDI-Out assignment retains External Step mode and its saved bindings. No non-MIDI Clip may retain a hidden active External Step mode.
- A Song containing any External Step Clip records the exact local compatibility sentinel `nl-save-schema-1`, even while the feature is Off or its input is unavailable.
- Schema-1 firmware accepts local schema 0 and 1, rejects future, malformed, or empty local schema sentinels, and otherwise preserves ordinary official and Community version comparison.
- Older External Step files guarded with `c1.3.1` remain loadable and resave with schema 1. Unsupported firmware refuses the guarded Song instead of silently loading the Clip on the Song clock. Remove the stronger compatibility requirement only after every External Step Clip returns to Song mode.

### Compatibility

- Preserve ordinary Song-clocked MIDI Clips, their editing, output channel, note and Control Change transmission, launch behavior, probability, iterance, automation, save data, and display behavior.
- Preserve the established meanings and priority of MIDI Learn, global commands, section and Clip commands, Kit-row mute controls, parameter mappings, MIDI Follow, instrument input, recording, and MIDI Thru whenever no active valid External Step binding owns the message.
- Do not apply External Step to Audio, internal Synth, Kit, CV, or Arrangement Clips. Do not add another standard MIDI Clock domain, Song Position Pointer, external Start or Stop ownership, MIDI clock output, linear recording, overdub recording, smooth automation, swing, arpeggiators, sample synchronization, or MPE timing.
- Pulse handling, timeout service, Reset, Stop, and cleanup perform no heap allocation, storage access, sample access, or work in the shared Song next-event scheduler.
- A disabled feature does not consume configured controls. Arrangement playback does not advance Session External Step Clips.

### Checks

- Test exact device, raw physical channel including MPE-configured ports, type, and number matching; Note and Control Change edge rearming; Program Change edges; independent and shared Step and Reset controls; Reset independence from Step validity; Reset-before-Step ordering; stopped-transport Reset.
- Check the first step at zero, later steps, loop wrap, off-grid starts, short notes, note-off ordering, collapsed starts, probability, iterance, and stepped automation.
- Check qualified timeout, irregular clocks, disconnect, feature disable, Step and Reset conflicts, invalid data, and save/load defaults.
- Check conflict entry and exit rearming, one cut on a newly conflicting Step, all configured Clips on Stop and Session exit, non-MIDI output demotion, and unsupported timing edits without an ordinary resume or any extra stored-note rewrite from the safety cleanup.
- Verify that Song ticks, resync, launch, live-position, arpeggiator, and shared next-event handling exclude External Step Clips; matching controls cannot leak into ordinary MIDI handling; no pulse path allocates or accesses storage; and Stop, Reset, timeout, feature disable, disconnect, conflict transitions, timing edits, and Panic cannot leave an owned note sounding.
- Check schema 0 and 1 acceptance, future and malformed schema refusal, unchanged ordinary-version comparison, and Songs saved with and without External Step Clips.
- Current firmware must reload both. An unsupported Community build must refuse only the guarded Song. Returning every affected Clip to Song mode must remove the stronger compatibility requirement.
- Load an older External Step file guarded with `c1.3.1` and verify that it resaves with schema 1.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical Deluge connected to VCV Rack, check Note, Control Change, and Program Change Step and Reset controls; independent and shared clocks; first step, wrap, Reset, Stop, Panic, clock loss, reconnect, Clip switching, UI feedback, save and reload, and ordinary Song Clips.

## 24. Release-order-safe preview-only note input

### Purpose

Hold Horizontal to audition notes or drums without recording them into the Clip. Use the same gesture in Instrument Clip View audition rows and Keyboard View, regardless of release order.

### Behavior

- In Instrument Clip View, holding Horizontal before pressing an audition-column pad makes that note preview-only for Kit, Synth, MIDI-Out, and CV Clips.
- In every Keyboard View layout that produces instrument notes, including Velocity Drums and melodic, chord, MIDI-Out, and CV layouts, holding Horizontal before a main-grid note press makes every resulting note-on or retrigger preview-only.
- A preview-only note sounds and releases through the same instrument, MIDI, CV, choke, arpeggiator, effect, velocity, and expression path as ordinary auditioning. It may select the same Drum or note row and show the same note, chord, layout, or recording feedback that does not claim a note was written.
- A preview-only note never records a note-on, count-in early note, retrigger, or note-off into the Clip. It does not create, extend, shorten, or close an existing sequencer note.
- Decide whether a note is preview-only at note-on or retrigger. Keep that decision through its matching note-off. Releasing Horizontal before the pad cannot create a recorded note-off without a matching recorded note-on.
- A note begun without Horizontal remains an ordinary recording gesture through its matching note-off. Pressing Horizontal after that note begins cannot suppress its required recorded note-off or turn the already sounding note into a preview.
- Repeated notes, overlapping physical pads that produce the same pitch, generated chord notes, encoder-driven note remapping, and Kit retriggers must pair sounding and recorded note starts with the correct releases. Leaving the view, stopping playback, changing Clips or Outputs, or invoking Panic cannot leave a preview note sounding or leave stale preview ownership that affects a later gesture.
- Preview-owned MIDI-Out and CV notes still transmit normally. Only Deluge Clip recording is suppressed.
- When Horizontal is not held as a note begins, audition and Keyboard recording retain their established behavior.

### Compatibility

- Preserve Instrument Clip note-grid editing, Horizontal zoom, scroll, note nudge, multiply, row rotation, clipboard shortcuts, and audition-row selection.
- Preserve every Keyboard layout's Horizontal rotation and pressed-Horizontal behavior, including Chord Library voicing, Velocity Drums scroll and zoom, and the separate Scale-modified velocity-profile controls.
- Preserve normal audition silence for an already sequenced row and for established Shift or Vertical modifiers. This deviation does not make a row audible when the existing conflict-avoidance rules require silence.
- Preserve resampling, MIDI Learn, external MIDI input, ordinary live recording, automation, undo history, project data, and firmware compatibility. The preview gesture adds no saved setting.
- Do not change Audio Clip recording, Song or Arranger launch behavior, Slicer audition, Waveform Editor audition, or external-controller note input.

### Checks

- Test preview and ordinary note ownership, both Horizontal and pad release orders, Horizontal pressed after an ordinary note-on, note retriggers, overlapping same-pitch pads, generated chords, encoder-driven remapping, count-in early notes, and cleanup when leaving or stopping.
- Verify that Clip View and Keyboard View decide ownership at note-on or retrigger, pair every sounding and recorded note-off with the correct start, keep preview output audible, and do not disturb Horizontal encoder controls.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical Deluge, check stopped audition, playback, armed recording, and count-in in Kit and melodic Clips; both release orders; Horizontal pressed after a normal note starts; repeated and overlapping notes; chords; held pads while turning Horizontal; MIDI-Out and CV output; view exit; Stop; and Panic. Confirm no stuck sound, stray note, shortened prior note, lost note-off, or changed encoder gesture.

## 25. Legible full-screen waveform playheads

### Purpose

Keep the playhead visible over dense or strongly colored full-screen waveforms. Add a playhead to the OLED sample-editing waveform without increasing display work beyond the limits below.

### Behavior

- Sample Marker Editor, Region and Manual/Lazy Slicer, Audio Clip View, and Sample Browser render the waveform at seven eighths of its established full intensity. Start and End bounds, loop and slice markers, undefined regions, controls, selections, and playhead colors retain their established strength and draw above the waveform.
- Show a playhead only for audio playing from the exact source displayed. An unrelated voice, an earlier preview, another Drum, another Clip, or a matching filename without matching sample identity must never drive it.
- Sample Marker Editor follows the newest active voice whose sample holder owns the displayed sample. Audio Clip View follows only the displayed active Clip. Sample Browser follows only its active browser-preview sample. Region and new-Kit Slicer follow only the browser preview, while Manual/Lazy and existing-Kit Slicer follow only the selected Sound.
- The Region Slicer pad cursor spans all eight waveform rows. The Manual/Lazy Slicer cursor spans only its four waveform rows and never covers its slice pads. Other full-screen pad cursors retain the height already owned by their waveform.
- Sample Marker Editor and both Slicer modes add a one-column OLED playhead to their existing waveform companion. It maps the raw source position into the complete visible waveform range, draws after every waveform contour and bound marker, and reverses the pixels in its column so it stays visible over both empty and lit areas.
- The OLED cursor moves only while the exact displayed source is actively playing and its projected display column changes. It disappears after stop, release, source replacement, view exit, or movement outside the visible range. Scrolling or zooming remaps it to the same source position without changing playback or resetting the playhead.
- Refresh the moving OLED cursor no more than twenty times per second through the existing display service. Do no OLED work while the projected column is unchanged. Update once when hiding or replacing the cursor to clear its old column.
- Audio Clip View keeps its existing whole-Clip OLED ruler instead of gaining a second waveform cursor. Sample Browser keeps its existing filename display and gains no OLED waveform.

### Compatibility

- Preserve waveform shape, sample analysis, color family, Clip and track color ownership, bounds, slice locations, viewports, scrolling, zooming, reverse playback, audition controls, transport behavior, recording, sample data, project data, and audio output.
- Preserve the established pad cursor color, bound and marker colors, Slicer control colors, grey undefined regions, and every non-waveform pad. Only the full-screen waveform base becomes dimmer.
- Preserve Session, Row, Grid, Arranger, recorder, transition, cached thumbnail, and single-row waveform brightness and cursor behavior. These surfaces show multiple sources or overviews, not a full-screen single source.
- Preserve all seven-segment behavior. Do not add a new display timer, background task, playback engine, persistent setting, saved data, or firmware compatibility requirement.
- OLED drawing consumes only the already cached waveform, marker, viewport, and projected playback state. It performs no sample scan, decoding, cluster load, storage access, memory allocation, audio rendering, or direct display transfer.

### Checks

- Test included and excluded surfaces, the seven-eighths waveform level, untouched overlay strength, exact source matching, newest matching voice selection, Region and Manual row ownership, first and last visible columns, offscreen positions, stopped and replaced sources, reverse movement, loop wrap, scrolling, zooming, and stale-cursor cleanup on exit.
- Check OLED one-column mapping, contrast over blank and lit waveform pixels, overlap with selected and unselected markers, visibility transitions, the twenty-hertz limit, unchanged-column suppression, viewport changes, and one final cleanup update.
- Verify that the browser, editor, Slicer, and Clip each use the correct playback source; excluded thumbnails are unchanged; dynamic display work is fixed and bounded; and no display path allocates, accesses storage or samples, renders audio, creates a timer, or sends the OLED directly.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical OLED Deluge, check each included surface with sparse and dense waveforms, forward and reverse playback, loop wrap, start and end edits, Slicer Region and Manual audition, browser preview, markers, popups, scrolling, zooming, stop, source replacement, and exit. Confirm that the waveform is still readable, the playhead is more obvious, no stale cursor remains, and audio, storage streaming, recording, and display response do not regress.

## 26. Non-destructive Song compatibility preflight

### Purpose

Reject Songs that require unsupported firmware or a newer local save schema before loading changes playback, the interface, Undo history, the current Song, or any other live project state.

### Behavior

- Before changing live state, read each XML or JSON Song only to check compatibility. Close the file after this check. Reopen it from the beginning only if it is compatible.
- Recognize the official and Community firmware fields and the exact local save-schema sentinel in the existing compatibility field. Local schema 0, 1, and 2 are supported. A future schema, malformed local sentinel, or empty local sentinel is rejected.
- Compatibility fields may appear anywhere in the Song root. Unknown fields before them, including nested arrays or objects and strings containing braces, brackets, escaped quotes, or escaped backslashes, cannot hide a later incompatibility marker or corrupt the second read.
- For canonical Songs with compatibility fields first, stop the initial read once the complete requirement is known. Legacy Songs without a compatibility field remain loadable after the root has been checked.
- An empty ordinary firmware-version value is treated as the established unknown version instead of causing an invalid memory access. Other invalid ordinary version strings retain their established unknown-version meaning.
- Reject an incompatible Song with its exact incompatibility error. Leave playback, the current Song, the active interface, Undo and Redo history, and all loaded state unchanged.
- A read, close, reopen, malformed-file, or storage-removal failure preserves its own error. Each successful file open has one matching close, and a failed reopen cannot reuse stale parser state or an earlier file handle.
- Repeat the normal load's existing compatibility checks after reopening. Report any compatibility error they find.

### Compatibility

- Preserve compatible XML and JSON Song loading, legacy Song loading, ordinary official and Community version comparison, browser behavior, file selection, error reporting, and the complete normal deserialization path after reopen.
- Kit presets and saved-row presets continue using their established compatibility reads and error propagation. This deviation does not add a second preflight to those non-Song files.
- Do not rewrite, repair, migrate, resave, or otherwise modify a refused file. Do not change Song contents, project formats, playback semantics, Undo semantics, or storage ownership beyond the compatibility-only first read and required reopen.

### Checks

- Test production XML and JSON readers with canonical and reordered compatible headers, too-new ordinary versions, local schemas 0 through 2, future schemas, malformed and empty sentinels, legacy files without markers, empty and invalid ordinary versions, malformed input, nested unknown payloads with structural characters inside escaped strings, and storage failure between the two opens.
- Verify that refusal occurs before changes to playback, the interface, Undo, Redo, or the current Song; compatible files close and reopen exactly once; and the normal reader still propagates a defensive incompatibility result.
- Format every affected source, compile the changed storage and Song-loading units with the target ARM Release toolchain, run the complete configured host suite, and complete a local Release firmware build.
- A physical Deluge check loads representative compatible, legacy, unsupported, malformed, and storage-interrupted Songs and confirms that every refusal leaves the playing project and Undo history intact.

## 27. Kit Sound Drum choke groups

### Purpose

Let a Kit contain independent groups of mutually exclusive samples. Use Polyphony `CHOKE` to enable group participation.

### Behavior

#### Membership and controls

- Each audio Sound Drum has a choke group from 1 through 16. Use group 1 for missing, zero, or invalid saved values. New Sound Drums and newly sliced rows begin with group 1 and their established non-CHOKE Polyphony default.
- Only Sound Drums set to Polyphony `CHOKE` participate. A Sound Drum retains its saved group while Polyphony is another mode, but it neither chokes nor is choked until Polyphony returns to `CHOKE`.
- With a selected Kit Sound Drum set to Polyphony `CHOKE`, pressing Select on that value opens a group selector from 1 through 16. OLED shows `Choke group` and values `1` through `16`; seven-segment displays show `CHGP` and `G01` through `G16`. Other Polyphony values, Synths, MIDI rows, Gate rows, and non-Kit contexts do not expose the selector.

#### Triggering

- Starting an ordinary audio Sound Drum in `CHOKE` mode releases every currently sounding `CHOKE` Sound Drum in the same Kit and same group before the replacement starts. This includes the triggering row's previous voice, so self-retriggers remain click-safe.
- Ordinary sequencer notes, held row audition, incoming Kit MIDI notes, and Cut, Once, Loop, and Stretch playback all use the same group behavior. Different groups, non-CHOKE Sound Drums, MIDI rows, and Gate rows never release one another.
- Release the group only when a normal trigger starts a Sound Drum immediately. Direct starts, Kit-arpeggiator bypass, one-shot fallback, and an immediate Kit-arpeggiator output release the group exactly once immediately before starting. Duplicate, deferred, probability-suppressed, or otherwise no-output Kit-arpeggiator inputs do not release the group. Later starts generated internally by that arpeggiator do not rescan or mutate Kit rows.
- Releasing a matching row stops only MIDI notes currently tracked as belonging to that row, resets that row's arpeggiator and inversion state so it cannot retrigger later, fast-releases its active audio, and updates its render eligibility before the replacement begins. It never sends channel-wide All Notes Off. Delay and reverb tails continue through the established fast-release behavior.
- The dedicated held-Select Kit Waveform Editor preview from deviation 19 does not trigger group release and is protected from unrelated group release while that dedicated preview owns the row. Normal held row audition remains a participating ordinary trigger.

#### Saved state and row operations

- Keep the Sound Drum's group through Song save and load, Kit presets, saved-row presets, full Sound Drum copy and paste, and row reorder. Notes-only operations do not invent or change group identity. Deleting a row removes that Sound Drum's group membership. Reordering needs no group remapping.
- Replacing an existing Slicer anchor Sound Drum preserves that Sound Drum's group and Polyphony according to the established in-place replacement behavior. Newly appended Slicer rows use group 1 and the ordinary non-CHOKE default.
- A stored group 2 through 16 requires local save schema 2 even when that Sound Drum is not currently `CHOKE`. Group 1 alone adds no new requirement. External Step alone requires schema 1; a file containing both records the maximum requirement, schema 2.
- Songs, Kit presets, and saved-row presets write a non-default group value and the appropriate compatibility marker. Schema-2 firmware accepts schemas 0, 1, and 2 and rejects future, malformed, or empty local schema sentinels. An unsupported build must refuse protected data rather than silently collapsing every row into group 1.

### Compatibility

- Preserve the established Polyphony meanings, ordinary Kit triggering, row and Kit arpeggiators, sample playback modes, MIDI and Gate behavior, effects tails, audition selection, Sound Drum clipboard behavior, row reorder and deletion, Slicer placement, and project data unrelated to the new group field.
- Do not add a pad shortcut, another Polyphony value, a second enable switch, a bulk group editor, a horizontal menu slot, group lighting, MIDI or Gate groups, channel-wide All Notes Off, global effects cancellation, or render-time Kit-list traversal.
- Scan groups only when an ordinary trigger is about to start a Sound Drum. It must not allocate memory, access storage, change Kit row membership, or run from an audio render or arpeggiator tick callback.

### Checks

- Test group normalization, group membership, self-retrigger, independent groups, inactive saved groups, exact tracked MIDI note-off cleanup, row-arpeggiator reset, ordinary-start versus internal-arpeggiator ownership, copy and clone behavior, persistence defaults, schema aggregation, menu visibility, and OLED and seven-segment formatting.
- Check Songs, Kit presets, saved-row presets, missing and invalid group values, group 1 without a stronger marker, groups 2 through 16 with schema 2 even while non-CHOKE, External Step alone with schema 1, combined features with schema 2, and refusal of unsupported or malformed schemas.
- Verify that reorder and deletion use existing Sound Drum identity, Slicer anchors preserve existing state, new Slicer rows use defaults, direct Waveform Editor preview remains protected, no channel-wide MIDI message is sent, and no cross-row scan occurs from render or tick callbacks.
- Format every affected source and localization input, regenerate localization through the established generator, compile the changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical OLED and seven-segment Deluge, check groups 1 and 2 in one Kit, self-retrigger, held audition, sequencer and incoming MIDI triggers, Cut, Once, Loop, Stretch, row and Kit arpeggiators, external MIDI echo shared with unrelated rows, delay and reverb tails, the dedicated Waveform Editor preview, copy and paste, reorder, deletion, Slicer anchor replacement, new slices, menu navigation, save and reload, schema refusal on older firmware, and malformed-file refusal.

## 28. Incremental keybinding catalog

### Purpose

Catalog physical controls from the source so players can look up forgotten shortcuts and maintainers can check consistency.

### Behavior

- The repository provides `docs/keybindings/bindings.jsonl`, with one independent JSON object per line. Add entries as bindings are reviewed; do not wait for a complete action inventory.
- Each record identifies the binding, applicable mode, physical gesture, semantic action and arguments, user-facing description, relevant conditions, controlled tags, search keywords, source files and symbols, and verification scope. Relevant timing, partial effects, release ownership, dispatch order, consumed failures, and cancellation remain visible.
- Multiple bindings for one action retain a shared action identity. Separate bindings and arguments remain distinguishable so contextual reuse and possible inconsistencies can be audited.
- The catalog declares its incomplete state and records uncertainty. Missing entries do not imply missing firmware functionality. Do not describe source review as hardware testing or proof that every input path is reachable.
- Coverage spans global dispatch, every view and keyboard layout, inherited handlers, sample editors and slicers, browser/load/save/rename controls, menus, parameter shortcut maps, DX7 and gold-knob behavior. Configurable controls are described as families rather than an invented finite list of user assignments. Disabled or non-user input paths have explicit exclusion reasons. Describe actual local behavior, including current-UI precedence before the Panic counter.
- Each physical control has one canonical identity. A companion registry explains aliases, the distinction between an encoder switch and rotation, coordinate spaces, and state-dependent pad roles. Holds, triggering events and ordered sequences remain distinguishable; timing and release ownership are not discarded when simplifying the displayed shortcut.
- A separate coverage ledger links reviewed handlers and relevant helpers to bindings or explicit exclusions. Source snapshots allow later audits to report changed files. Specific inconsistencies are recorded separately from the firmware behavior being documented.
- Document the format and named conditions so another agent can add compatible entries. A future search view or report consumes this catalog rather than maintaining competing handwritten records.

### Compatibility

- Catalog changes do not alter firmware controls, audio, stored project data, or dispatch behavior. Discovering a discrepancy does not authorize changing that behavior.
- Keep code and pseudocode examples in the catalog documentation rather than in this deviation contract. Hardware testing is outside this documentation change.

### Checks

- Every line parses as one JSON object, binding IDs are unique, and referenced files and source anchors are checked against the described checkout.
- Review representative timing, release, held-control, error, and fallback cases. Report the exact number of catalogued records and known omissions; do not infer completeness from that count.
- Reconcile discovered input definitions with the coverage ledger, reject unresolved gaps and missing binding references, and detect changes in every referenced source file, including mapping and control-definition sources. Treat matching gestures as review candidates, not proven conflicts without their conditions and dispatch order.
- Test accepted and rejected gesture forms, duplicate identifiers, uncovered handlers, unresolved review gaps, source drift and physical-control normalization. Keep discovery limitations and untested hardware behavior explicit.

## 29. Shared Song Root / Mode menu

### Purpose

Show and edit the Song's musical key in one compact Scale menu. Keep view-specific Scale controls.

### Behavior

- In idle melodic Instrument Clip, Keyboard and Clip Automation views, Shift + Scale opens a menu containing Root and Mode instead of the former immediate scale-cycle action. The same gesture opens the shared Song menu in idle Session Rows and Grid, Arranger, Arranger Automation and Performance views.
- Root selects one of the twelve pitch classes. Use the established Song root-selection behavior, which preserves note pitches and may infer a different mode from the existing notes. After an edit, read both values from the Song. Root and mode are linked; do not silently transpose the Song.
- Mode selects an enabled preset or an available saved User scale using the existing Song scale-mapping and validity rules. Display the actual current mode, including custom state, without changing it merely to fit the list. Rejected changes leave the previous musical state intact.
- Select rotation chooses Root or Mode, Select press enters value editing, and rotation edits the selected value. Back leaves value editing first, then returns to the originating view. OLED shows the two items and their actual values; seven-segment hardware uses readable names and values through its established display conventions.
- Consume the opening Scale press through its release, even if Shift is released first or the menu has already closed. One menu-opening gesture must not also cycle a scale or toggle Scale mode.
- Keep Root and Mode compact near the top of the OLED, where the recessed display remains readable. Pads remain undimmed. Pass every pad event and Vertical turn to the underlying native view, preserving that view's modifiers, playing, editing, scrolling and navigation. Do not create a separate audition mode or pad-release policy. Only refresh this menu's display if it remains current after native input.
- Refuse Root and Mode edits while pads or native editing gestures are active. Show a brief request to release the pads, preserving the mapping used when they were pressed. Do not stop held notes or synthesize releases to make a key edit possible.
- Preserve ordinary undo for native actions through the menu, returning to the originating musical view rather than to the menu. If the current keyboard layout cannot represent the edited key, keep that key and select a compatible layout with visible feedback instead of silently replacing the scale.
- Opening, viewing and closing the menu do not change root, mode, Clip scale-mode membership, notes, playback or undo history. No-op and rejected edits do not clear undo. Actual accepted edits retain the established musical semantics and existing undo limitations; this feature does not add scale-edit undo support.

### Compatibility

- Preserve Velocity Drums Scale velocity-profile and fixed-velocity controls, all other Kit Scale behavior, and Sound Editor horizontal-menu page/group navigation. Preserve higher-priority Learn, Load, pad and pressed-encoder combinations.
- Do not add this menu to an existing menu, browser, sample editor, Slicer or recorder, or use an unrelated current Clip as an implicit Song target. Do not implement Scale-modified Horizontal or Vertical encoder shortcuts as part of this menu.
- Keep root and mode in their existing Song state. Add no saved field, save schema, timer or audio-path processing. Playing audio continues; opening the menu must not retrigger notes or change recording behavior.

### Checks

- Check supported and excluded views, extra held controls, both Shift/Scale release orders, closing before Scale release, and ordinary Scale behavior after returning.
- Check all roots, current/custom/disabled/User modes, invalid scale transitions, no-op edits and fresh Song state. Verify that opening and rejected edits preserve undo, and that displayed values reflect the actual Song after root inference or scale mapping.
- Run focused input and menu-state tests, source formatting, the configured host suite, affected target compilation, independent review and one uninterrupted logged local Release build. Check navigation, display readability and live playing behavior on OLED and seven-segment hardware separately with the operator.

## 30. Fine envelope time controls

### Purpose

Make short envelope transients easier to adjust without changing existing patches or engine timing.

### Behavior

- Apply fine adjustment to Attack, Decay and Release of all four ordinary sound envelopes. Sustain retains its established value range and controls; DX7 operator envelopes and the sidechain envelope are outside this feature.
- Show the familiar envelope amount with two decimal places, from 0.00 through 50.00. An unaccelerated Select turn changes one unit per detent. Holding Select while turning changes one hundredth of a unit per unaccelerated detent. Preserve finer stored values instead of first rounding them to the displayed value. Keep existing Shift and turn-speed acceleration where the surrounding menu applies them; Shift alone does not activate fine editing.
- On OLED, also show the nominal unmodulated stage duration in milliseconds or seconds. Label it as a base time, not the duration of the sounding voice. Preserve immediate attack and do not display a nonzero duration for it.
- Use the finer amount consistently in full-screen and horizontal menus, notifications, and seven-segment displays. Existing parameter navigation and modulation-patching gestures retain their meanings.
- Keep the graphical envelope overview and its stage markers within their existing screen area across the full Attack, Decay, Sustain and Release ranges. Whole-unit settings retain their previous graph shape; fine edits retain fractional precision until pixel placement. Sustain keeps its original scale.

### Compatibility

- Opening, viewing or leaving the menu must not change the stored parameter. Zero movement or an outward turn at an endpoint must not change the value. Clamp edits safely at the original endpoints, including large encoder offsets.
- Old Songs and presets retain their exact parameter values and sound until edited. Preserve ordinary automation editing, MIDI learning and feedback, Kit Affect Entire, parameter modulation and save compatibility.
- Do not change the audio engine, envelope curves, minimum stage times or saved parameter format. Fine editing exposes intermediate settings. It does not lower the engine's minimum stage times or make stage transitions sample-accurate. Live modulation and block timing can differ from the nominal readout.

### Checks

- Test coarse/fine increments, endpoint saturation, zero-input preservation, off-grid values, monotonic time conversion, immediate attack and representative short and long durations. Verify that Sustain and non-envelope controls remain unchanged.
- Check both menu layouts and display types, patching shortcuts, automation and Kit Affect Entire. Run focused tests, the configured host suite, target compilation and an uninterrupted logged Release build. Physical readability and audible transient behavior require operator confirmation.

## Maintaining this document

Keep this file in the newest local feature revision. Add or update a contract whenever a deviation changes. Each contract must name where the change applies, what it does, what stays unchanged, its edge cases, and how to check it. Keep enough detail to recreate the feature without patch history, private plans, or conversation context. Describe the required result, not code, pseudocode, or an implementation recipe.
