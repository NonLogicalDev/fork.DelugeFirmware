# Local deviations from upstream Deluge Community Firmware

## Purpose and baseline

This document defines the complete local deviation stack for an agent beginning with a fresh checkout of upstream `main`. Recreate every contract below before considering that checkout equivalent to this local build. The entries describe outcomes, boundaries, and verification only. They intentionally do not prescribe source-level design, algorithms, or code.

The local stack currently contains twelve deviations:

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
- The environment provides a checkout-local compiler cache under the ignored `.cache/ccache` path. C and C++ compilation uses that cache while every cache miss still uses the same locked v22 cross-toolchain.
- A local Release build completes and produces the normal firmware binary at `build/Release/deluge.bin`.

### Compatibility and verification

- The environment is developer tooling only. It does not change firmware behavior or package a flashing workflow.
- The toolchain version and all Nix inputs are locked so a later agent can reproduce the same environment from a fresh upstream checkout.
- Cache state is local and disposable. It is not committed, uploaded, shared with another checkout, or treated as a firmware artifact.
- A newly configured build and any existing build after one reconfiguration show `ccache` as their C and C++ compiler launcher. Cache statistics may be inspected after a build; a first build may have only misses.
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

## 5. Kit sample-creation entry and reuse

### Intent

Let a player add a folder, region slices, or manual slices safely to the top of an existing Kit, beginning at the selected Sound Drum.

### Required behavior

- When the sample browser is creating a kit from a selected audio file, its action menu contains three choices: Load all, Slice, and Manual slice.
- In a brand-new Kit, Manual slice is available under the same selected-file condition as the existing Slice action. It is not offered for a folder.
- In an existing Kit, Load all, Slice, and Manual slice are available when the selected pad contains a Sound Drum and no assigned Kit row is above it. The selected Sound Drum, whether it is empty or already has a sample, becomes the first generated row. Every additional generated row is appended above it.
- A selected MIDI or Gate Drum, or a selected Sound Drum with an assigned row above it, cannot start any of the three actions. The player receives localized feedback that the top empty Kit pad is required.
- Each existing-Kit Slicer entry remains in the mode chosen from the menu. Its mode-switch control does not change modes during that reuse session, so the selected anchor and sample source stay valid.
- Choosing Manual slice retains the selected file as the selected anchor's playable source, stops the separate browser preview, and opens the existing Slicer directly in its established Manual mode. Its waveform and slice-pad audition work without depending on the browser preview. It otherwise retains the same initial one-slice state, pad editing, slice count, transpose, save, confirm, and cancel behavior that Manual mode already provides.
- Choosing regular Slice from a valid existing-Kit anchor retains its established region-slicing behavior while placing every new row above that anchor. Before either Slice mode opens on an existing Kit, the selected file becomes that anchor's source and the Slicer waveform comes from that source, not a previous browser preview. Load all retains filename-based row naming and applies the same anchor-and-above placement rule.
- A confirmed Slice or Manual slice longer than two seconds uses the configured default sample mode. A slice shorter than two seconds uses Once so very short fragments do not choke later hits. Temporary Manual Slicer audition may also use Once while the player is editing.
- The top three pads in Slicer's far-right status column choose the pending batch mode in vertical order: Default, All Cut, All Once. The selected pad is visibly brighter and gives immediate display feedback. The main slice grid and the far-right audition column keep their existing roles.
- Default retains the normal duration-based behavior. All Cut makes every newly confirmed slice in that batch use Cut, including short slices. All Once makes every newly confirmed slice in that batch use Once. These choices never change the stored device default or existing Kit rows.
- A Manual Slicer audition may temporarily use Once, but cancelling Slicer restores the selected anchor's prior mode. Only confirmation commits a generated batch's mode.
- Each confirmed Slice or Manual slice batch receives a series name and zero-padded part number: the first is `A-01`, `A-02`, and onward. A later batch uses the series after the highest existing generated series, such as `B-01`, then `C-01`, with `AA-01` after `Z-01`.
- Existing row names are never changed. A pre-existing row whose name already has the generated-series form reserves that series. Legacy numeric slice names do not reserve a lettered series.
- Previewing or cancelling Manual Slice never changes a row name. Names are assigned only after the player confirms the slice batch.
- Leaving Manual Slice with Back retains its established behavior: the new anchor remains as one full-sample pad. Existing Kit rows remain unchanged.
- Entering either Manual slice or the existing Slice action requests the Slicer's initial grid redraw immediately. The player does not need to move an encoder or send another input before the waveform and Slicer grid appear.
- Choosing the existing Slice action continues to open the Slicer in its established Region mode with its existing initial slice count and controls.
- The Manual slice selection applies only to that entry into the Slicer. A later normal Slice entry must still begin in Region mode.
- The new menu label is localized for both OLED and seven-segment displays.

### Compatibility and boundaries

- Do not add a new slicing algorithm, slice-detection heuristic, kit type, sample format rule, maximum slice count, or save format.
- The valid selected-top-Sound-Drum rule is the only existing-Kit reuse route for Load all, Slice, and Manual slice. Do not allow insertion into the middle of a Kit.
- Do not move, replace, clear, or reorder existing Kit rows below the selected Sound Drum. Keep their notes, sound settings, and playback state unchanged.
- Do not change the Slicer mode-switch control for a brand-new Kit, kit playback, MIDI, project saving, or the established short-slice Once rule. Generated slices longer than two seconds must follow the configured default sample mode.
- The three Slicer batch-mode controls apply only while Slicer is open and only to the batch about to be confirmed. They do not alter device defaults, other sample-loading paths, or existing Kit rows.
- The new choice is a faster route to an existing mode. It does not persist as a global default.

### Verification contract

- A human runtime check on a physical Deluge should confirm that holding Select on a chosen audio file opens the Manual slice action without a timing-sensitive shortcut; the Manual Slicer waveform and its slice pad audition the selected sample; standard Slice still starts in Region mode; and a normal entry after Manual slice is still Region mode. It should also confirm that a second batch started from a selected top Sound Drum, with a different selected file, shows and uses that new file rather than a tail or preview from the earlier batch; that the selected Sound Drum receives the first slice, later slices appear above it, existing rows below remain unchanged, and a Sound Drum with an occupied row above is refused. In both modes, check Default, All Cut, and All Once with long and short slices; confirm that a cancelled Manual audition restores its earlier mode.
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

- In a brand-new Kit, the existing Slice action and Load all action retain their current entry and preview behavior. Existing-Kit Slice source ownership is defined by the Kit reuse contract above.
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
- Related Kit Sound Drum row-copy work uses the `kit-rows` group.
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
- Five physical Shift presses, each beginning no more than 500 ms after the previous one and with no intervening button or grid-pad press, invoke that same Panic action. The fifth press gives the existing stopped feedback and starts a new gesture sequence.
- Selecting Panic stops active playback, scheduled audio events, instrument notes, audio clips, MIDI and gate notes, auditions, and sample-browser preview sound.
- Panic removes active delay, stutter, modulation, compressor, filter, and reverb runtime state so no existing effect tail or feedback continues after the normal short output-buffer latency.
- The Panic routine immediately aborts active audio recording or stem export whenever a caller invokes it. An unfinished capture is intentionally not preserved, because emergency silence takes precedence.
- Panic gives the existing localized stopped feedback and keeps the Settings menu usable for a later action.

### Compatibility and boundaries

- The first Panic entry remains available through the normal-operation Settings menu. The five-Shift gesture is an additional global physical-button entry; recording and stem export retain their established controls until a later local deviation exposes a dedicated menu action in those modes.
- Fewer than five Shift presses, a gap longer than 500 ms, or any intervening button or grid-pad press does not invoke Panic. Encoder turns do not affect the gesture.
- The first four Shift presses retain normal Shift behavior. The fifth is reserved for Panic and leaves Shift off so the emergency action does not leave a modifier latched.
- Panic changes only live audio and unfinished capture state. It does not change saved songs, presets, samples, sequences, automation, settings, MIDI configuration, or undo history.
- Panic does not directly clear the raw hardware output buffer. Any already queued output may finish during the normal short device latency; no new sound or effect tail may follow it.
- Existing transport controls, menu navigation, audio routing, and device installation behavior remain unchanged until Panic is selected.
- Panic is local-only. It does not authorize flashing, installation, publication, or upstream submission.

### Verification contract

- The host unit-test suite and a local Release build pass.
- A physical Deluge check confirms each of Load all, Slice, and Manual slice adds from a valid selected top empty pad upward without changing existing lower rows. It also confirms two slice batches display distinct series names, Manual preview and cancellation leave names unchanged, and save/reload preserves the names.
- A physical Deluge check verifies silence after invoking Panic during each currently reachable normal-mode scenario: a playing synth or kit, an audio clip, a sample-browser preview, a MIDI or gate note, delay feedback, stutter, modulation effects, reverb, a resonant filter, and compressor-driven audio. When a later caller exposes Panic during recording and stem export, validate the existing abort behavior in those modes too.
- The same check confirms no project content, settings, automation, or undo history changed after Panic outside the intentionally aborted unfinished capture.

## 10. Copy, paste, and reorder a Kit Sound Drum row

### Intent

Let a player duplicate a Kit Sound Drum, its sound settings, and its musical row to another pad without rebuilding the sample setup by hand, and safely correct an accidental Kit-row move.

### Required behavior

- In a Kit Clip View, holding one assigned Sound Drum's Audition pad and pressing Learn copies that instrument only. The source remains unchanged.
- The first copy contains the Sound Drum's sample or synthesis definition, sample start and end points, loop settings, sound parameters, and MIDI mappings, but does not include ordinary row notes or row playback defaults.
- A second Learn press within the normal short double-press interval while the same source remains held replaces the local copy with the Sound Drum plus its ordinary row notes and playback defaults such as mute state, loop length, direction, probability, iterance, fill, and colour.
- Holding the target Kit pad with Audition, Shift, and Learn pastes the local copy. A full copy replaces the target's Sound Drum, notes, and row defaults; an instrument-only copy replaces only the target's Sound Drum and its sound parameters, leaving that target's notes and row defaults intact.
- The two Learn uses remain distinct by order: Shift + Learn before an Audition target begins the established MIDI-unlearn flow, while Audition target + Shift + Learn requests Kit-row paste. Once MIDI Learn is active, Kit-row copy and paste do not take over its Learn input.
- The target may be an empty Kit pad or a pad already assigned to a Sound Drum. A paste to an empty pad creates the target row. A paste to an existing Sound Drum stops that target before replacing it.
- The pasted Sound Drum is independent of the source. Later changes to either drum's source file reference, sample markers, looping, sound parameters, notes, or row settings do not change the other.
- Source and target must belong to the currently open Kit in the current song. The copy is not a saved, cross-song, or cross-Kit clipboard.
- MIDI and Gate drums are not valid sources or replacement targets. Report that an occupied target is unavailable rather than modifying it.
- Give clear copy-mode, copy-complete, paste-complete, allocation-failure, and unavailable-target feedback on both OLED and seven-segment displays.
- In a Kit Clip View, holding an assigned source row's Audition pad, then an assigned destination row's Audition pad, and pressing the Vertical encoder moves the source to the destination position and shifts the intervening rows. Both rows remain usable as their moved rows after the gesture.
- Each successful reorder creates one normal Undo action. Undo restores the exact preceding row order and Redo reapplies the same move. Earlier Undo history remains available.
- Reordering changes row position only. Each row keeps its assigned instrument, notes, row settings, automation, and independent identity.
- When the gesture does not identify two distinct currently assigned Kit rows, it does not create an Undo action or change row order.

### Compatibility and boundaries

- Plain Audition plus Horizontal Encoder continues to edit a row's length. Holding the Horizontal Encoder continues to rotate a row. Learn plus the Horizontal Encoder retains its ordinary note and automation clipboard behavior; it does not trigger this local Kit-row action.
- The operation does not alter Kit master settings, other rows, source data, sample files, project saving, MIDI playback, or hardware installation behavior.
- Sound and row-parameter automation are not copied. A successful paste clears the existing undo history rather than offering a partial or unsafe undo operation.
- Do not change the established reorder gesture, replace it with drag-and-drop, or extend reordering across clips or Kits.
- Do not add cross-Kit persistence, MIDI/Gate cloning, hardware flashing, installation, release publication, or upstream submission.

### Verification contract

- A source review confirms that all resources for a prospective target are ready before that target is changed, and that a failed allocation leaves the destination unchanged.
- The host unit-test suite and a local Release build pass.
- A physical Deluge check confirms a full copy to an empty target and an existing Sound Drum target, exact sample-marker and loop transfer, independent later edits, full note transfer, sound-only note preservation, MIDI/Gate rejection, ordinary Horizontal Encoder behavior, local display feedback, and save/reload behavior. It also confirms Kit-row reorder, Undo, Redo, and an earlier unrelated Undo action remain correct. No flashing or installation is authorized by this contract.

## 11. Optional incoming MIDI sustain pedal

### Intent

Let a standard sustain pedal control the live release of notes played from an external MIDI controller into an internal Deluge Synth, including a DX7 sound, without changing existing MIDI behavior until the player chooses it.

### Required behavior

- Community Features contains a persisted `MIDI Sustain Pedal` setting that defaults to Off.
- With the setting Off, incoming CC64 and incoming note-offs retain their established behavior.
- With the setting On, an incoming CC64 value from 64 through 127 holds the live release of notes that the player releases from a matched external MIDI input on an internal Synth track. A value from 0 through 63 releases only those previously released notes.
- A note that remains physically held when the pedal is released continues sounding normally. If the player replays a pitch before releasing the pedal, a later pedal release must not stop the new held note.
- The feature applies through the ordinary internal Synth input path, so DX7 sounds gain the same pedal behavior without a DX7-only setting, sound-file field, or preset conversion.
- MIDI Learn receives CC64 before this behavior. A player can still learn or unlearn CC64 normally, and a normal learned CC64 mapping may continue to receive its value.
- Turning the setting Off releases any notes already held by the pedal. Stopping or replacing the receiving Synth, and Panic, also clear the pedal-held state so it cannot leave a later note sounding.
- Clip recording retains the physical note-off timing. This deviation does not record pedal events or change saved song, synth, or automation data.

### Compatibility and boundaries

- Do not add half-pedal response, sostenuto, pedal recording, a new MIDI mapping format, or a global MIDI filter.
- Do not apply this behavior to Kit, Audio, CV, or external MIDI-output tracks. Sequenced notes, grid-pad audition, external MIDI output, existing sound envelopes, DX7 synthesis, and Community Feature settings other than this opt-in toggle retain their established behavior.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- A local Release build and the configured host test suite pass.
- A physical Deluge check with a CC64 controller verifies the setting defaults Off; pedal-down and pedal-up behavior for a single note, chord, repeated pitch, and a still-held pitch; DX7 behavior; normal MIDI Learn and unlearn; a learned CC64 mapping; target change; Stop; Panic; and save/reload. No flashing is implied by this check.

## 12. Song Grid existing-track column glow

### Intent

Make the Song Grid's existing track columns visible before a clip is launched or selected, while preserving the established color and brightness language for clip state.

### Required behavior

- In the Grid layout of Song View, every visible main-grid column that represents an existing track has a neutral-white base in its otherwise empty cells.
- The base is monochrome at brightness 7 on the 0 through 255 LED scale. It is a structural cue, deliberately much dimmer than ordinary clip colors.
- A column that does not represent an existing visible track remains black.
- An actual clip cell continues to render its existing color and brightness over the base. Inactive, active, playing, selected, armed, solo, record, MIDI-learn, and pulse feedback retain their existing appearance and precedence.
- Scrolling changes which track columns receive the base according to the existing visible Grid mapping.

### Compatibility and boundaries

- Do not add a setting, animation, color theme, persistent state, or a new clip state.
- Do not change clip launch, selection, creation, duplication, deletion, scrolling, zooming, section behavior, track order, session data, MIDI, audio, or existing side-column controls.
- The base applies only to the Song Grid's main pads. Other session layouts and unrelated views retain their current appearance.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- A local Release build and the configured host test suite pass.
- A physical Deluge check compares an empty existing-track column, an empty non-track column, inactive clip cells, active or playing clip cells, and a scrolled Grid. It confirms that the structural base is visible but subordinate to every established clip-state color and brightness. No flashing is implied by this check.

## 13. Repeated Audio Looper recording safety

### Intent

Let a player record repeatedly into the same Audio Clip in `Looper/FX` mode without emptying the Clip, corrupting the active recorder, or crashing on a later pass.

### Required behavior

- An empty Audio Clip may use the established first-take recording flow in which Record followed by Play starts without the project clock and stopping the take derives the project tempo.
- After that Audio Clip contains a sample, another recording pass into the same `Looper/FX` Clip uses the established project clock. A populated Clip must never re-enter the first-take tempo-establishing flow.
- Stopping a valid later pass replaces the Clip's prior sample with the completed recording. The Clip remains present in the same song location, retains its activation and `Looper/FX` monitoring role, and is immediately usable for playback or another recording pass.
- The prior sample remains assigned until the replacement has completed far enough to be installed. Cancelling a pass, capturing no samples, reaching the file-size limit, or rejecting the recorder before completion must leave the prior sample assigned and the Clip usable.
- Recorder attachment and detachment are exact-owner operations. A recorder may detach only itself from the Output that currently owns it. A failed attempt to attach a second recorder must not detach, abort, or replace the recorder already attached to that Output.
- Recorder detachment is safe when repeated and safe after the recorder has already finished or detached. Destroying an unattached or stale recorder must not change another recorder or its Output.

### Compatibility and boundaries

- Preserve continuous microphone or line-input monitoring in `Looper/FX`, the existing first-take tempo calculation, normal Clip playback, Clip naming after a successful recording, and the established recording controls.
- Preserve ordinary Audio Clip cloning and overdub behavior outside `Looper/FX`, including non-Looper inputs, Arrangement recording, and recordings into a newly created Clip.
- Do not delete, move, deactivate, or recreate the destination Clip as part of a successful replacement. Do not clear other Clips, their samples or automation, unrelated project data, or prior undo history.
- This deviation does not change the separate cloned-overdub path, redesign Song Grid or Row looping, make late storage failures transactional, add a recording mode, change audio routing or file formats, repair unrelated held-Clip cancellation gestures, flash firmware, publish a build, or submit an upstream change.

### Verification contract

- Focused host checks cover empty-versus-populated first-take eligibility and exact-recorder Output detachment where the host harness can represent those objects. The complete configured host test suite and a local Release firmware build pass.
- On a physical Deluge, create a new Audio Clip, select microphone or line input with Audio Output set to `Looper/FX`, and complete the first Record + Play + Play-stop take. Confirm that it establishes tempo and remains playable.
- In that same Clip, complete at least ten further Record + Play + Play-stop passes. After every pass, confirm that the Clip remains present, contains the latest completed take, continues monitoring as before, and can immediately begin the next pass without freezing or crashing.
- During separate later passes, cancel before completion and stop once without captured audio. Confirm that the previous completed take remains assigned and playable.
- Physical verification remains a human-controlled step and does not authorize flashing or installation.

## Maintaining this document

Each new local deviation adds or updates a contract in this file in the newest local feature revision. A completed document must remain understandable without local patch history, owner plans, or conversation context. It must identify the affected surface, exact required outcome, behavior that must remain stable, meaningful edge cases, and the evidence needed to verify the deviation. Do not add code, pseudocode, or implementation recipes.
