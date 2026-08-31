# Local deviations from upstream Deluge Community Firmware

## Purpose and baseline

This document defines the complete local deviation stack for an agent beginning with a fresh checkout of upstream `main`. Recreate every contract below before considering that checkout equivalent to this local build. The entries describe outcomes, boundaries, and verification only. They intentionally do not prescribe source-level design, algorithms, or code.

The local stack currently contains twenty-seven deviations:

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
- Confirming two or more slices in a brand-new or existing Kit creates exactly one playable Sound Drum per requested slice. Every generated row retains its assigned sample bounds, transpose, mode, and batch name; no later slice may be omitted because its row object was not retained during confirmation.
- A confirmed Slice or Manual slice longer than two seconds uses the configured default sample mode. A slice shorter than two seconds uses Once so very short fragments do not choke later hits. Temporary Manual Slicer audition may also use Once while the player is editing.
- The top three pads in Slicer's far-right status column choose the pending batch mode in vertical order: Default, All Cut, All Once. The selected pad is visibly brighter and gives immediate display feedback. The main slice grid and the far-right audition column keep their existing roles.
- Pressing Select in either Region or Manual Slicer opens a fixed Auto, Cut, Once confirmation menu. Its initial highlight matches the pending batch mode already shown by the far-right controls on both OLED and seven-segment devices.
- Turning Select in that menu changes only the highlight. Pressing Select on an item closes the menu, applies that choice to the same pending batch state used by the far-right controls, and immediately runs the existing slice-confirmation action.
- Pressing Back from the menu returns to the exact in-progress Slicer session without changing its source, mode, boundaries, slice count, transpose values, preview state, or pending batch mode. If slice confirmation fails, Slicer remains open with the accepted mode and its existing failure feedback.
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

- A human runtime check on a physical Deluge should confirm that holding Select on a chosen audio file opens the Manual slice action without a timing-sensitive shortcut; the Manual Slicer waveform and its slice pad audition the selected sample; standard Slice still starts in Region mode; and a normal entry after Manual slice is still Region mode. It should also confirm that a second batch started from a selected top Sound Drum, with a different selected file, shows and uses that new file rather than a tail or preview from the earlier batch; that the selected Sound Drum receives the first slice, later slices appear above it, existing rows below remain unchanged, and a Sound Drum with an occupied row above is refused. In both Slicer modes and on both display types, check that Select opens the Auto, Cut, Once menu at the pending right-pad choice; turning Select does not change that choice; Back restores the unchanged session; and accepting each mode confirms long and short slices with the required playback behavior. Also confirm that a failed confirmation returns to the same session with the accepted mode and existing error, and that a cancelled Manual audition restores its earlier mode.
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
- A confirmed defect that predates the local feature stack uses its own bookmark named `ns/bug-<component>-<NN>`. Related defects reuse the component slug and increment `NN` within that component.
- A defect introduced by a local feature does not receive a later bug bookmark. Its correction replaces or folds into the originating `nl/patch-*` revision so the stack contains only the intended final implementation.
- Related Manual Slicer work uses the `sample-slicer` group in stack order: direct Manual entry, editing controls, then direct-entry preview stopping.
- Related Keyboard View chord-display work uses the `kb-chords` group.
- Related Kit Sound Drum row-copy work uses the `kit-rows` group.
- A distinct core capability may use its own group. The Panic feature uses the `panic` group rather than the incidental Settings menu that exposes it.
- The repository-compatibility revision remains `local/fixups` at the bottom of the local stack, before user-facing revisions.
- When a new decision replaces earlier local behavior, fold it into the affected feature revision and update this document. Do not retain a later revision solely to cancel obsolete behavior, source, or contract text.
- Use Jujutsu, not Git commands, for local repository inspection, revision management, and diff review.

### Compatibility and verification

- This convention changes local revision names and organization only. It does not change firmware behavior, build outputs, installation, flashing, or upstream state.
- A local Jujutsu bookmark listing shows one sequential series for each feature area, core capability, or pre-existing bug component, with no duplicate feature-specific group for the same area.
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
- Pedal-held note-off handling for all 128 MIDI pitches requires no new memory allocation on the live event path. Low-memory conditions cannot throw, reset, or turn a released pitch into a stuck note; disabling the feature and every target-cleanup path retain the same guarantee.
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

## 14. Song Grid and Audio Clip track color

### Intent

Give each Audio track one recognizable color across Song Grid and Audio Clip waveforms instead of maintaining unrelated track and clip colors.

### Required behavior

- The Song Grid track color is the authoritative hue for every Audio Clip attached to that Audio Output.
- Every full-screen and single-row Audio Clip waveform renders a pastel form of that hue while retaining its established sample-shape and amplitude brightness.
- A newly created Audio Clip uses its track color on its first render. A new clip in an existing Grid column immediately matches that column, and multiple Audio Clips attached to one Audio Output always share one waveform hue.
- Changing an Audio track's color through the Song Grid updates every waveform attached to that track on its next render. Fine and coarse Grid color changes never leave the track in the reserved unassigned-color state.
- The established Shift plus Vertical Encoder color gesture in Audio Clip View changes the shared Audio track color. The equivalent Shift plus held-clip gesture in Row Song View does the same and redraws every visible Audio Clip on that Output.
- An in-place overdub keeps the existing Output color. An overdub that creates a new Audio Output receives and then follows that new Grid column's color.
- The shared Output color survives save and reload through the established project format. Legacy per-clip color data remains readable and writable for compatibility, but it is only a fallback while an Audio Clip is temporarily unattached from an Output.

### Compatibility and boundaries

- Do not change Instrument Clip note colors, Kit-row colors, section colors, Arranger clip-instance colors, or the Song Grid's active, inactive, playing, selected, armed, solo, recording, MIDI-learn, and pulse brightness behavior.
- Do not change waveform shape, amplitude analysis, sample data, audio playback, recording, overdub routing, track order, clip placement, or undo history.
- A project whose Audio Output has no assigned color acquires one nonzero track color before its waveform is shown. The reserved unassigned value must never appear as a temporary visible red waveform or cause a later Grid render to choose a different hue.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused host checks cover positive and negative color stepping, hue-range wraparound, accelerated steps, and crossing the reserved unassigned value. The complete configured host test suite and a local Release firmware build pass.
- A physical Deluge check covers a fresh Audio track, a second clip in the same Grid column, Grid fine and coarse color changes, both Audio color gestures, multiple visible Row clips sharing one Output, an in-place overdub, an overdub-created Output, save and reload, and an older project whose Audio Output color is unassigned.
- The physical check confirms the waveform stays in the same hue family as its Grid column while remaining visibly pastel, and that every excluded color and brightness system remains unchanged. No flashing is implied by this check.

## 15. Long-file waveform peak addressing

### Intent

Keep waveform navigation and peak lookup correct across the recorder's supported file range instead of allowing signed 32-bit sample, byte, or cluster arithmetic to wrap and hide the waveform or select unrelated data.

### Required behavior

- Waveform peak collection accepts visible sample positions beyond `INT32_MAX` and derives non-negative byte and cluster positions without signed wrap, including positions around the 2 GiB and 4 GiB byte boundaries.
- Loaded samples use their recorded audio-data byte length when determining the valid end of the waveform. A live recorder derives its valid byte length from the captured sample count and complete sample-frame width without overflowing the conversion.
- Each visible column retains a signed sample interval and an unsigned file interval until the code has proved that a narrower index is valid. A request that overflows, precedes the sample, exceeds the valid audio range, or cannot address the sample's cluster collection is treated as unavailable or beyond the waveform; it must not index a negative or wrapped column, byte, or cluster.
- Zooming, scrolling, pinning a marker, and calculating the rightmost legal viewport remain stable throughout the supported sample length. Zero-length samples open at a valid minimum zoom instead of underflowing.
- Existing project fields for a saved sample-editor viewport remain signed 32-bit values. Save a viewport only when both values are representable and the zoom is positive; otherwise save the established unset values so reopening uses the full view instead of a wrapped or invisible view.

### Compatibility and boundaries

- Preserve waveform shape, extrema, short-file navigation, marker pinning, sample playback, recording, project format, and the established behavior of representable saved viewports.
- Do not expand the supported media format, recorder size limit, cluster container, or persistent viewport fields as part of this deviation.
- Do not allocate memory, load extra clusters, or add background work merely to handle wide positions. Existing failures to make sample data available remain retryable through the established renderer path.
- Reversed row mapping is governed separately by deviation 16. OLED presentation is governed separately by deviation 17.

### Verification contract

- Focused host checks cover signed addition with negative, zero-crossing, maximum, and overflowing positions; byte positions immediately below and at 2 GiB and 4 GiB; sample positions above `INT32_MAX`; invalid frame widths; byte multiplication overflow; and invalid cluster-size magnitudes.
- Source review confirms that loaded and live-recorded lengths take their respective checked paths, all narrowing occurs after explicit bounds checks, and a final-cluster byte limit is calculated from the full valid audio byte position.
- Compile the waveform renderer, basic navigator, and sample-marker editor with the target ARM Release toolchain, then complete the configured host suite and a local Release firmware build.

## 16. Reversed half-open waveform row ranges

### Intent

Make a reversed single-row waveform mirror the requested display interval without producing a negative source column, dropping an edge column, or reading outside the fixed peak arrays.

### Required behavior

- Treat every row request as a half-open interval `[start, end)`. Mirroring within a display of width `W` maps it to `[W - end, W - start)`.
- Reversing the full 16-column pad interval therefore remains `[0, 16)`. Reversing either one-column edge interval stays one column wide and inside the display. Partial intervals retain their width and mirror to the opposite side.
- Reject a peak-collection request before touching cached column state when its start is negative, its end precedes its start, or its end exceeds the display width.
- After a valid reversed lookup, output columns continue to read the corresponding mirrored source peaks and retain the established amplitude and color treatment.

### Compatibility and boundaries

- Preserve all forward waveform rendering, peak analysis, sample and recorder access, display width, Audio Clip color, collapse animation, and row ownership.
- Do not clamp or silently reshape an invalid caller range. Report failure through the renderer's established incomplete-result path so the caller cannot mistake invalid cached data for a finished waveform.
- This contract applies to the generic single-row pad renderer. OLED marker and contour presentation remains a separate deviation.

### Verification contract

- Focused host checks cover the full display, both one-column edges, and several partial half-open intervals, asserting width preservation and in-bounds mirrored endpoints.
- Source review confirms that range validation precedes every `colStatus`, minimum, or maximum array access and that reversed output columns use the matching mirrored source column.
- Compile the affected waveform renderer with the target ARM Release toolchain, run the focused host checks together with the long-file regression, and complete the configured host suite and local Release build.

## 17. OLED sample-editing waveform companions

### Intent

Show sample shape and the edited region on an OLED Deluge without adding sample analysis, storage traffic, audio work, background animation, or repeated display transfers while the view is unchanged.

### Required behavior

- `SampleMarkerEditor` shows a monochrome outline of the currently visible sample region beneath a compact marker header. This applies to instrument Sample start, end, loop-start, and loop-end editing and to Audio Clip start and end editing.
- The selected sample marker is a clear full-height bound. Other relevant start, loop, and end markers remain visible as smaller ticks without covering the waveform.
- Horizontal scrolling and zooming update the OLED waveform and marker positions only after the visible region changes. Reversed samples preserve the same left-to-right relationship as the pad waveform.
- The Slicer owns its OLED page in both Region and Manual/Lazy modes instead of appearing as a small overlay on the sample browser.
- Region mode shows a compact region-count header, the sample outline, and equal-region divisions only when they remain far enough apart to read as distinct marks.
- Manual/Lazy mode shows the selected slice number, slice count, and current start value with the sample outline. All slice starts remain visible as small ticks, while the selected slice start and end are clear full-height bounds.
- Adjusting a Manual/Lazy start keeps the waveform visible; temporary text feedback must not cover the waveform. Slice selection, creation, deletion, count, mode, boundary, and successful viewport changes update the OLED when their displayed state changes.
- The OLED outline defaults to 128 independently measured peak ranges across its 128 columns, mapping one range to each physical column and providing eight times the 16-pad horizontal resolution. A build-time setting may select 16, 32, 64, or 128 equal display ranges without changing the visible editing behavior outside waveform detail and analysis cost.
- Each group of adjacent OLED ranges that occupies one pad column covers exactly the same half-open sample interval as that pad column, including when the visible sample count does not divide evenly into display ranges. When enough distinct source samples are visible to address every range independently, each sample is measured by the same range that its horizontal display projection occupies. At tighter zooms, adjacent ranges deliberately share sample ownership rather than implying detail that does not exist. Ranges with identical half-open sample ownership reuse one collected peak result instead of repeating sample analysis.
- If cached waveform amplitude is unavailable for part of the view, every successfully measured part remains visible while the unavailable part stays blank and can be retried by the normal editing redraw path. OLED rendering must not initiate sample analysis, decoding, cluster loading, storage access, memory allocation, or audio-engine servicing to fill it.
- The companion consumes the generic renderer behavior from deviations 15 and 16: valid long recordings remain visible when zoomed out, and reversed views remain inside the same visible half-open range as the pad waveform.
- A static editor schedules no waveform-driven display refreshes after its current state has been drawn. Except for the source-matched playhead required by deviation 25, playback, audition, marker blinking, and periodic graphics routines do not animate or refresh the OLED waveform.

### Compatibility and boundaries

- Preserve the pad waveform's viewport, layout, colors, markers, and navigation, plus marker limits, slice calculations, playback, audition, saving, audio processing, control gestures, and project format. On the two OLED editing surfaces, each pad column may show more precise extrema from the same visible region; this refinement must not trigger a second sample-analysis pass.
- Preserve all 7SEG behavior. Do not add an OLED waveform to Sample Browser, idle Audio Clip View, Session, Arranger, recording screens, or other unrelated views.
- Except for the bounded source-matched cursor in deviation 25, do not add a playback cursor, stereo lanes, filled amplitude bars, inverted selection, a project-persistent or Audio-Clip-wide waveform cache, a timer, or a background waveform task.
- OLED waveform rendering remains display-only and must never run from an audio-rendering path. A physical power comparison is required before claiming a battery-life improvement.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused host checks cover the supported display-range counts, exact viewport and pad-column boundary equivalence at remainder-heavy zooms, intentional shared ownership at tight zooms, one-column default range placement, silence, full-scale and asymmetric peaks, reversed views, partial availability and retry, beyond-waveform columns, first and last columns, vertical amplitude bounds, and deterministic fixed work for the cached input width. The generic long-address and reversed-range checks remain owned by deviations 15 and 16.
- Source review confirms that OLED rendering consumes only already-available peak data, allocates no memory, performs no sample or storage work, adds no graphics or playhead timer, and invalidates OLED only after relevant editor state changes.
- Compare the baseline and changed Release ELF sizes, run the complete configured host test suite, and complete a local Release firmware build.
- On a physical OLED Deluge, check instrument Sample start, end, loop-start, and loop-end; Audio Clip start and end; zoomed, scrolled, and reversed views; Slicer Region divisions; and Manual/Lazy slice selection and boundary movement. Leave each editor idle while audio, storage streaming, CV output, and optional OLED mirroring are active, and confirm the waveform remains readable without visible display churn or new audio interruption. This check does not authorize flashing.

## 18. OLED Clip timeline ruler

### Intent

Give a player a compact visual reference for progress through the complete Clip loop, its beat grid, and the part currently visible on the pad matrix without displacing the OLED information already used for sound, Clip, and recording feedback.

### Required behavior

- Normal Instrument Clip View for Synth, Kit, MIDI Out, and CV Clips, normal Audio Clip View, every non-Arranger Clip Automation surface, and every Keyboard View layout show a monochrome ruler in the three display rows at the OLED's top visible edge. Supported Automation surfaces include Automation Overview, parameter automation, and note Velocity editing for Instrument and Audio Clips where those modes apply. Supported Keyboard layouts are Isomorphic, In Key, Piano, Chord, Chord Library, Velocity Drums, and Norns.
- The ruler's 128 columns always span the complete Clip loop from time zero through its exclusive end. Horizontal scroll, zoom, and Triplet layout changes must not rescale this whole-Clip domain or reset the playhead when it leaves the 16-pad view.
- In timeline Clip and Automation views, a separate clipped span shows the part of the Clip visible on the 16 main pad columns. Horizontal scroll moves that span, zoom changes its width, and a view covering the complete Clip fills the ruler. A pad view entirely outside the Clip shows no false selection. Keyboard View never shows this span because its pad columns represent playable notes, chords, or drums rather than Clip time.
- The top row shows played progress and the bottom row shows musical marks. In timeline Clip and Automation views, the middle row shows the visible pad range; in Keyboard View it remains empty except where the Clip caps or live playhead cross it. Quarter-note marks are one pixel wide. Bar marks are two adjacent pixels wide; a bar at the right edge shifts inward instead of being clipped to one pixel. Clip start and end caps and the live playhead cross all three rows and are drawn over those lanes.
- Quarter-note marks appear when they remain at least four pixels apart across the complete loop. At greater density the ruler uses bar marks and then evenly coarser bar intervals. Marks remain bounded and never merge into a solid block.
- While the current Clip is actively playing or recording, a three-pixel playhead crosses the ruler and a one-pixel progress segment reaches from Clip time zero to that position. The indicator follows forward, Reverse, and Ping-Pong motion, continues beyond the visible pad range, traverses the OLED once per complete loop, and wraps only at the Clip boundary.
- A stopped Clip retains its whole-Clip caps, musical marks, and visible-range selection but does not show moving progress. Count-in and an inactive Clip likewise show no false live position.
- A cloned overdub shown against its source Clip follows the same repeated-position and direction behavior as the established pad playhead.
- During clocked linear Arrangement Audio recording, the ruler uses the growing live recording extent as its provisional whole-Clip length instead of the maximum-length sentinel, while the playhead remains at the growing right edge. A tempoless first-loop recording has no settled musical length, so it shows an explicit full-width progress and right-edge playhead state without beat marks or a false viewport selection until the musical length is established.
- Kit Clips with independently looping rows show the master Clip timeline. The single OLED ruler must not imply that all independently looping rows share another row's position.
- Normal OLED notifications and popups remain visually authoritative over the ruler. Entering Performance View, Arranger Automation, a menu, Sound Editor, Song or Arranger View, Audio Recorder, Sample Browser, Slicer, Sample Marker Editor, stem export, or a view transition removes or suppresses the live ruler update.
- Static ruler changes appear immediately after Clip change, scroll, zoom, transport state change, or recording-state change. Moving playback and growing-recording projections occur no more than 20 times per second and only when the visible result changes, including when unrelated OLED content causes a full display redraw between cadence checkpoints.

### Compatibility and boundaries

- Preserve every existing Clip title, parameter value, icon, popup, side scroller, stem-export display, pad playhead, pad color, navigation gesture, Clip timing rule, recording rule, and project format. In Keyboard View, preserve every layout, playable-pad behavior, recording tick, chord name, latest physical note, layout feedback, and temporary popup.
- Preserve 7SEG behavior and every OLED surface outside normal Instrument and Audio Clip views, non-Arranger Clip Automation, and Keyboard View.
- The ruler is display-only. It must not add a separate timer, allocate memory while updating, read samples or storage, scan a Clip's events, run from an audio-rendering path, send an unchanged display frame, or increase per-Clip saved or runtime state.
- Do not add a Song-level transport ruler, waveform, per-row Kit ruler, playhead trail, animation outside active playback or recording, user setting, color option, flashing, installation, publication, or upstream submission.

### Verification contract

- Focused host checks cover complete-loop mapping; ordinary, scrolled, zoomed, clipped, and Triplet-derived viewport ranges; quarter, two-pixel bar, right-edge bar, and coarsened marks; maximum supported timeline positions; forward, Reverse, Ping-Pong, wrapped, stopped, playing, and recording positions beyond the pad view; cloned overdubs; clocked Arrangement growth; tempoless first-loop state; exact three-row bounds and lane separation; unchanged-state suppression; full-render cache reuse; and the 20 Hz moving-update ceiling.
- Source review confirms that normal Instrument and Audio Clip views, non-Arranger Clip Automation, and Keyboard View own the ruler; Automation and Keyboard repaint it after replacing the OLED canvas; Keyboard uses no timeline viewport selection; inactive delegated Clip views do not invalidate another supported surface's shared cached frame; existing musical and viewport sources remain authoritative; all mapping work is fixed and bounded; and OLED animation adds no timer, allocation, storage or sample access, audio-path work, direct display transfer, or unchanged redraw.
- Compare baseline and changed Release ELF sizes, run the complete configured host test suite, and complete a local Release firmware build.
- On a physical OLED Deluge, check Synth, Kit, MIDI Out, CV, and Audio Clips in their normal Clip views and in non-Arranger Automation Overview, parameter automation, and note Velocity editing while stopped, playing, recording, wrapping, reversing, zooming, and scrolling. Check all seven Keyboard View layouts while stopped, playing, recording, counting in, switching layouts, showing chord and latest-note feedback, opening and closing a menu, and returning to Instrument Clip View during playback. Confirm Keyboard has no viewport-selection span, Performance View and Arranger Automation remain unchanged, titles and popups remain clear, movement is readable without dominating the screen, and simultaneous CV output, audio playback, recording, and storage streaming show no new interruption. This check does not authorize flashing.

## 19. Waveform Editor exact Kit-row audition

### Intent

Let a player hear the exact Kit Sound Drum whose sample bounds they are editing without leaving Waveform Editor or finding that row's current screen position.

### Required behavior

- In a Kit Sound Drum's Waveform Editor, pressing and holding the Select Encoder auditions the Sound Drum being edited. Releasing Select ends that held audition through the Sound Drum's established playback behavior. Modes that track a held note receive their matching note-off; Once and other no-tail behavior retain their established completion semantics.
- The audition uses the Sound Drum's current sample Start, End, loop bounds, reverse, transpose, and Cut, Once, or Loop playback settings. The waveform's visible scroll and zoom range do not become playback bounds.
- The audition remains attached to the Sound Drum that started it. Vertical Kit scrolling, waveform navigation, marker editing, or a later change to the selected Kit row must not redirect its release to another row.
- The dedicated preview intentionally stays outside the Clip-level Kit arpeggiator. It enters the edited Sound Drum directly through that Drum's normal note path, including its own arpeggiator, sample playback, effects, and the parameter state of the row that began the preview. This prevents the Kit arpeggiator from treating the preview as a numbered Clip row.
- A Kit active-Clip change ends and resets the dedicated preview before the new Clip becomes active, including Once and other no-tail playback. Releasing the earlier Select hold afterward is harmless and must not affect the new Clip.
- Select does not start or layer a dedicated preview while that same Sound Drum still has any active voice, including a Once or Cut voice playing to completion and an envelope release tail. Once the Sound Drum has become silent, Select can start the preview normally. This makes a later active-Clip hard stop exclusive to voices created by the dedicated preview.
- If the original Clip's rows are reordered or its preview row disappears while that Clip remains active, release still targets the original Sound Drum and uses only that Clip's row parameter state. It must not release the row now occupying the old position, leave the Sound Drum's own arpeggiator input held, or borrow parameter state from another Clip. If the original parameter state no longer exists, the preview is stopped and reset rather than released through unrelated state.
- While Select remains held, the dedicated preview receives the same choke protection as an established held row audition. Other choke-group activity must not silence it. Once the preview ends, is replaced, or is hard-cancelled, that protection ends immediately and normal choke behavior resumes.
- Select Encoder rotation retains its existing marker-editing behavior while Select is held.
- Pressing a normal row Audition pad while the dedicated preview is sounding stops the dedicated preview before the row Audition action begins. If a normal row audition is already active, Select must not start a competing preview.
- A later sequenced note, normal row audition, or MIDI audition of the same Sound Drum replaces the dedicated preview before the new note begins. Releasing the earlier Select hold after that replacement is harmless and must not stop the newer note.
- The preview follows the Kit's actual active Clip. If the edited Clip cannot become active, Select may audition the same Sound Drum through another active Clip only when that active Clip contains a row assigned to it. While the project clock is active, an active row that is sequenced remains silent so the preview cannot override sequenced playback.
- Leaving Waveform Editor, including through Back, stops any dedicated preview. Repeated release or exit handling is safe and cannot leave preview ownership latched.
- Panic or another external all-stop invalidates the dedicated preview. Releasing the earlier Select hold afterward must not send a stale note-off that cuts a later MIDI or row retrigger of the same Sound Drum.
- If storage work defers the Select action, no preview begins until the deferred press is actually handled, and its deferred release remains able to stop it.

### Compatibility and boundaries

- Preserve normal row Audition pads, Sound Drum selection, marker movement, waveform scroll and zoom, recording, MIDI input, saved project data, and song transport.
- Do not use the visible Kit row position to identify the preview target and do not record a note merely because the dedicated preview is used.
- The action applies only to a Kit Sound Drum's Waveform Editor. Sampled Synth and Audio Clip Waveform Editors retain their established playback controls, and modified Select gestures retain their existing behavior.
- The preview must preserve the Sound Drum's own arpeggiator, choke, effects, sample bounds, and playback-mode behavior. Only the Clip-level Kit arpeggiator is bypassed; unrelated sequenced voices must not be cut.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Compile the affected Waveform Editor source with the target ARM Release toolchain, run the configured formatting checks and host test suite, and complete a local Release firmware build.
- On a physical Deluge, enter Waveform Editor from a sampled Kit row, scroll the Kit so that row is no longer at its former screen position, and confirm that Select still auditions and releases the edited Sound Drum.
- Check marker changes while holding Select, Start and End bounds, reverse, transpose, Cut, Once, and Loop, the Sound Drum's own arpeggiator, choke groups and effects, a pre-existing row audition, switching to a normal row Audition pad, repeated Select presses and releases, Back while sounding, and a storage-busy deferred press and release. Start the same Sound Drum normally in Once and Cut modes, and during an audible release tail, then press Select: no dedicated preview may layer over the existing sound. After the Drum becomes silent, Select must preview it normally. While the dedicated preview is held, trigger another choke-group Sound Drum and confirm the preview remains protected; after releasing or replacing it, confirm normal choke behavior resumes. With the clock running, confirm a sequenced active row remains silent. When the edited Clip cannot become active, confirm the same Sound Drum is previewed only if the actual active Clip owns a row for it. Schedule another Clip on the same Kit to launch while Select is held, including Cut, Once, and Loop previews and a destination Clip where that Sound Drum occupies another row index and plays a note. The preview must stop before the active Clip changes, the destination note must start normally, and the later Select release must not target or cut it. Repeat after reordering the original Clip's rows, and remove the original preview row while keeping the Clip active to verify exact-Clip release or the hard-stop fallback without borrowing another Clip's parameters. While Select is held, retrigger the same Sound Drum from a row pad and from MIDI; each new audition must replace the dedicated preview, and the later Select release must not cut it. After Panic, retrigger the same row from MIDI before releasing Select and confirm the stale release does not cut that retrigger. Confirm that no note is recorded and no preview remains stuck.
- Confirm that sampled Synth and Audio Clip Waveform Editors, modified Select gestures, row audition, recording, and transport remain unchanged. Physical verification remains a human-controlled step and does not authorize flashing or installation.

## 20. Recoverable Audio Clip Start editing

### Intent

Let a player trim an Audio Clip from its playback Start and later recover source material from the original audio file without relying on Undo or leaving Audio Clip View.

### Required behavior

- When the existing `Trim From Start Of Audio Clips` Community Feature is On, Audio Clip View shows the playback Start as a faint green boundary and End as a faint red boundary. Material before playback Start is visibly dimmer than material inside the Clip.
- If the source file contains audio before playback Start, ordinary Horizontal scrolling can reveal that material as negative Clip-time editing space. The space contains only real source audio and ends exactly at the recoverable source boundary. It is not silence and does not play until Start is moved into it.
- Every other timeline surface retains a zero-or-later minimum scroll position. Entering Session, another Clip type, or another view from negative Audio Clip space restores a legal position for that surface.
- Tapping the green Start or red End pad selects that playback boundary. The selected boundary blinks in its own color. Turning Select without Shift moves only the selected boundary at the current horizontal zoom resolution.
- If Start and End occupy the same pad column, the shared boundary is shown as a dim yellow marker when neither is selected. Pressing that column selects Start first and then alternates Start and End. When the column immediately to its right exists, pressing it selects End directly.
- With no boundary selected, Select retains Audio Output mode selection. Shift plus Select retains Audio Output assignment. Ordinary Horizontal rotation always scrolls and pressed-Horizontal rotation always zooms, even while a boundary is selected.
- Clockwise movement advances a boundary later in playback time and counterclockwise movement moves it earlier. Forward and reversed Clips therefore behave the same to the player even though playback Start and End correspond to opposite raw file boundaries when reversed.
- Moving either boundary changes the Clip's musical length by the same anchored source-to-tick ratio. One encoder gesture is calculated from the values present when that gesture began, so repeated detents do not compound rounding drift.
- Start can expand only into recoverable source audio and can trim only until one source sample and one musical tick remain. End retains its established ability to trim or expand into source material. Neither boundary can cross the other or exceed the maximum supported Clip length.
- A fast turn that reaches a raw source edge or sequence-length edge lands on the exact legal boundary. The first reverse detent must move back toward the interior without requiring the acceleration overshoot to be unwound.
- Moving Start keeps the visible source position anchored as the Clip rebases to musical tick zero. Moving End retains its established visible-space behavior.
- One uninterrupted run of Select detents on one boundary is one Undo action. Undo and Redo exchange the exact raw boundary and musical Clip length together. Switching boundaries, using another control, changing Clip or view, or otherwise ending the gesture closes that action.
- A boundary edit is rejected before changing either value when recording or count-in is active, storage owns the data path, reversible history cannot be allocated, the Clip or source state is invalid, or the result would be illegal. A rejected edit neither creates an empty Undo action nor destroys existing Redo history.
- During ordinary playback, accepted edits leave the raw boundary, Clip length, automation, Arrangement instances, and resumed playback mutually consistent. Repeated Undo and Redo must not destructively trim automation before its saved state is restored.
- When `Trim From Start Of Audio Clips` is Off, Audio Clip View does not expose negative pre-Start space or a Start-selection affordance. Existing End selection, relocation, scrolling, zooming, and output controls remain available without allowing a Start-pad press to collapse the Clip.

### Compatibility and boundaries

- Preserve the audio file, sample data, recording, monitoring, launch behavior, time-stretch choice, Output assignment, Output mode selection, existing End relocation gesture, Clip color, waveform analysis, and project format.
- Do not add stored negative time, prepend silence, modify Waveform Editor, or expose pre-Start space in Instrument, Kit, MIDI, CV, Automation, Session, Song, or Arranger views.
- Marker colors and directions are playback-relative. Reversal must not expose the wrong source edge or swap the meaning presented to the player.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused host checks cover forward and reversed Start and End movement, exact raw-edge saturation, sequence-length saturation, minimum length, wide source positions, half-tick rounding, cumulative gesture ratios, fast-boundary reversal, source-backed negative-scroll limits, and signed scroll and zoom navigation.
- Source review confirms explicit Start or End selection, modifier ownership, feature-Off compatibility, coincident-marker selection, gesture-closing boundaries, recording and storage guards, allocation-safe history creation, exact coupled Undo and Redo, automation consequence ordering, and non-Audio scroll clamping.
- Compile every affected Audio Clip, timeline, waveform, action-history, and consequence source with the target ARM Release toolchain; run the complete configured host suite and a local Release firmware build.
- On a physical Deluge, check forward and reversed Clips; source-backed scrolling; Start and End trim and recovery; fast turns into each edge followed by immediate reversal; one-tick and same-column markers; Shift and both encoders; active playback; normal recording, count-in, and storage-busy rejection; repeated Undo and Redo with automation; feature On and Off; and navigation into Session and other Clip types. Physical verification remains human-controlled and does not authorize flashing.

## 21. Explicit Waveform Editor entry and bound selection

### Intent

Keep whole-sample information and the graphical Waveform Editor as two predictable layers: show sample information first, enter the graph deliberately, and return without losing the selected sample boundary.

### Required behavior

- Every supported Waveform, Sample Start, and Sample End launcher for Audio Clips, sampled Synths, and Kit Sound Drums opens its existing sample-information or sample-settings screen first. It does not draw the graphical Waveform Editor during entry.
- The relevant Waveform or boundary item is focused on that information screen. Pressing Select without Shift opens the graph. Pressing Shift plus Select on the information screen does not open it.
- Back from the graphical editor returns to the exact parent information screen and focused item. A subsequent Back follows the established menu path.
- The graph renders every active sample boundary that falls inside the visible source range. Sample Start and Sample End remain visible together; loop boundaries remain governed by their established availability. The controlled boundary is visually dominant and the others remain distinguishable.
- Sample Start is the initial controlled playback boundary. Leaving with Back and reopening the same sample preserves the chosen Start or End boundary. Editing a different sample resets the selection predictably to Start.
- One Shift plus Select press in the graph switches the controlled playback boundary between Start and End. The action toggles once on the press and consumes the matching release even when Shift is released before Select. It never selects a loop boundary.
- Boundary names, colors, and switching remain playback-relative for reversed samples.
- Turning Select retains the established movement rules for the controlled boundary. Ordinary Horizontal rotation scrolls and pressed-Horizontal rotation zooms. Marker-pad selection and loop-boundary behavior remain available.
- In a Kit Sound Drum's Waveform Editor, unmodified held Select retains its dedicated momentary audition behavior. Shift plus Select changes the controlled boundary without starting, stopping, or latching that audition.
- Invalid or missing samples and storage-busy entry retain their established refusal or deferral behavior. A rejected entry or switch leaves no stale selection and cannot consume a later unrelated Select press.
- OLED and seven-segment displays both identify the controlled boundary using their established display capabilities. Returning to information and reopening must not change sample data, bounds, playback, or the visible waveform range.

### Compatibility and boundaries

- Preserve raw sample-bound semantics, loop-point creation and removal, marker movement limits, waveform resolution and caching, scrolling, zooming, reverse playback, transpose, playback mode, audition, recording, Kit row ownership, Slicer behavior, project data, and audio processing.
- This contract does not change Audio Clip View, its Clip-time Start and End controls, negative pre-Start navigation, or Clip musical length. Those belong to deviation 20.
- Do not create a second information screen, merge the two editors, force offscreen bounds into view, add persistent edge indicators, or change sampled Synth and Kit playback behavior.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Source review covers every supported launcher and parent-menu shape, Audio Clip and sampled Synth or Kit context, multi-range samples, reverse playback, same-sample re-entry, different-sample reset, storage deferral, modifier release order, loop-boundary preservation, and Kit audition ownership.
- Compile every affected menu and graphical editor source with the target ARM Release toolchain, run formatting checks, run the complete configured host suite, and complete a local Release firmware build.
- On a physical OLED and seven-segment Deluge, check each launcher, information-to-graph Select, graph-to-information Back, the next Back, both visible bounds, Shift plus Select in both release orders, Start and End movement, loop markers, forward and reverse playback, same-sample re-entry, a different sample, Kit held-Select audition, and storage-busy behavior. Physical verification remains human-controlled and does not authorize flashing.

## 22. Velocity Drums velocity profiles

### Intent

Let a player keep large Velocity Drums hit areas while choosing how many distinct velocity targets each drum exposes. The velocity profile belongs to the existing layout rather than creating another drum layout or changing drum placement.

### Required behavior

- Velocity Drums offers four profiles named `FULL`, `4`, `2`, and `FIXED`. Each Clip remembers its active profile and fixed velocity independently of other Clips.
- `FULL` is the default for new Clips and for projects that do not contain valid profile data. It preserves the established velocity and brightness of every cell exactly.
- In a four-by-four drum block, `4` divides the pad into four two-by-two quadrants. The bottom-left quadrant emits velocity 32, bottom-right emits 64, top-left emits 96, and top-right emits 127. Every cell in one quadrant has the same brightness and emitted velocity.
- `2` divides a multi-row drum block into lower and upper halves at velocities 64 and 127. A one-row block instead uses left and right halves at those velocities.
- `FIXED` makes every cell in the drum block emit the saved fixed velocity. The value is limited to 1 through 127 and defaults to 64 when saved data is missing or invalid.
- When a block cannot form all requested regions, use its available horizontal or vertical cells to form no more than the selected number of ordered soft-to-loud regions. A one-cell block uses the saved fixed velocity.
- For `4`, `2`, and `FIXED`, each cell's brightness represents the velocity that the same cell will emit. Disabled drum blocks remain unlit, and an active note retains the established active-note dimming.
- In a Kit Clip using Velocity Drums, holding Scale and turning Horizontal selects the profile. Holding Scale and turning Vertical changes the fixed velocity and selects `FIXED`. Accelerated encoder offsets remain bounded or wrap only within the four profiles.
- Pressing and releasing Scale without an encoder turn reports the current profile instead of showing the Kit scale refusal. The matching release remains consumed even if the Output or Keyboard layout changes while Scale is held.
- A Load chord must not change or report the velocity profile. Scale-modified encoder actions take precedence over Shift color editing, pressed-Horizontal zoom, and Shift zoom only while Velocity Drums owns the Scale press.
- A profile or fixed-velocity change applies to later pad presses. It does not retrigger, stop, rewrite, or change the velocity of an already sounding or recorded note.
- Profile and fixed-velocity fields load independently. An invalid field falls back to its own default without resetting the other field or the Clip's existing Velocity Drums scroll and zoom state.

### Compatibility and boundaries

- Preserve drum order, block geometry, scroll, zoom, selected-drum tracking, retrigger choice, recording, note-off behavior, Clip color editing, and the complete legacy `FULL` velocity and brightness behavior.
- Preserve the established Scale button behavior in every non-Kit surface and every Kit Keyboard layout other than Velocity Drums. Leaving and returning to Keyboard View must not leave a stale Scale gesture.
- Do not repair the pre-existing widened final block mismatch at the three-cell horizontal zoom or the pre-existing zero-velocity edge at the largest `FULL` zoom as part of this deviation.
- Do not add a duplicate Keyboard layout, a timer, background work, persistent Song-wide state, Kit-row sample state, sequencer note editing, MIDI velocity processing, or the separate proposed note-length gesture.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused host checks cover independent saved-field validation, fixed-value clamping, exact `FULL` passthrough, four-by-four quadrant orientation, two-half orientation, fixed velocity, one-cell and single-axis blocks, odd and wide geometries, nonzero new-profile velocities, and brightness mapping.
- Source review confirms that `FULL` retains its separate legacy render path; Scale ownership, Load suppression, and encoder precedence cannot leak to another layout; profile changes do not touch active notes; and saving and loading use the existing per-Clip Keyboard state without disturbing scroll or zoom.
- Format every affected source, compile the changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete a local Release firmware build.
- On a physical Deluge, check every profile and zoom, four-quadrant orientation, two-region orientation, fixed-value limits, accelerated turns, OLED and seven-segment feedback, held notes, retriggering, scroll, zoom, color editing, Load chords, layout changes while Scale is held, and save and reload. Physical verification remains human-controlled and does not authorize flashing.

## 23. Externally stepped MIDI Clips

### Intent

Let the Deluge act as several independent MIDI step sequencers for modular software such as VCV Rack. Each eligible Clip follows its own learned external Step and Reset controls instead of deriving its position from the Song tempo.

### Required behavior

- Community Features contains an External Step MIDI Clips setting that defaults to Off.
- A Session MIDI-Out Clip exposes a Clock setting with Song and External Step modes. External Step remains a clock mode of the existing MIDI Clip rather than another Output type.
- Existing and newly created MIDI Clips remain in Song mode unless the player deliberately selects External Step while the Community Feature is On.
- A persisted External Step Clip retains access to its Clock settings while the Community Feature is Off so the player can return it to Song mode. The Clip remains stopped until then and never falls back to Song timing.
- External Step provides separate learned Step and Reset controls. Each control identifies one exact connected MIDI input, raw physical MIDI channel, message type, and number. Raw channel matching remains exact when the input port is configured for MPE; MPE zones do not widen or translate the binding. Supported message types are Note, Control Change, and Program Change.
- A valid Step control is required before the Clip can advance. Reset is optional and evaluated independently of Step assignment, connection, conflict, and Clip timing eligibility. An unassigned, missing, invalid, or conflicting Reset disables only Reset and does not block an otherwise eligible Step. A valid nonduplicate Reset remains usable when Step is invalid.
- For a Note control, one positive-velocity Note On creates an edge and later Note On messages do nothing until Note Off or a velocity-zero Note On rearms it. For a Control Change control, crossing from values 0 through 63 into 64 through 127 creates an edge and later high values do nothing until a value below 64 rearms it. Every matching Program Change creates one edge.
- A Clip cannot assign the same exact control to both Step and Reset. The same Step or Reset control may be shared by several Clips. One shared Step edge advances each eligible matching Clip once, and one shared Reset edge resets each active matching External Step Clip with a valid Reset once, even when that Clip's Step is invalid.
- When one message is Reset for some Clips and Step for others, every matching Reset occurs before any matching Step.
- Active MIDI Learn always receives a prospective binding first and learning a control never invokes it. Existing global, section, Clip, Kit-row, and parameter mappings retain priority. A conflicting External Step control remains saved but inactive. A Step conflict makes Step unavailable; an optional Reset conflict disables and reports Conflict only for Reset while Step remains governed by its own status.
- Whenever a higher-priority mapping changes a Step or Reset binding into or out of Conflict, that binding's held Note or Control Change state rearms. Program Change has no held state and remains one edge per matching message. A newly conflicting Step cuts the Clip's owned notes once while preserving phase; repeated conflict checks do not repeat the cut.
- While the feature owns a valid active binding, its matching Note, Control Change, or Program Change messages do not audition a sound, record notes or automation, enter MIDI Follow, control an Output, or pass through MIDI Thru.
- Global Play enables External Step advancement but the Song clock never moves an External Step Clip. Launching or relaunching the Clip places it before step zero and shows that it is waiting.
- The first accepted Step edge after launch, Stop, or Reset emits the events at Clip position zero exactly once without first advancing. Each later accepted Step edge advances by exactly the selected step.
- Reset works while transport is running or stopped and does not depend on Step validity. It cuts notes owned by the Clip, returns the Clip to the position immediately before step zero, clears cadence state, and emits no step-boundary note-on or automation. It does not rearm a held Note or Control Change; that control still needs its defined release or low value. The next accepted running Step emits position zero.
- Clip stop, mute, replacement, MIDI-Out reassignment, and feature disable cut notes owned by the Clip, rearm its Note and Control Change controls, and return it to launch state. Global Stop and leaving Session apply that cleanup to every configured External Step Clip, active or inactive. Cleanup emits no step-boundary note-on or automation; a note-off needed to cut an owned note is the only permitted MIDI output.
- Panic cuts owned notes and clock-loss state while preserving the current phase and held-edge state. The next Step after Panic continues from that frozen phase unless the Clip is also stopped or reset.
- Supported step sizes are one quarter, one eighth, one sixteenth, and one thirty-second note. The saved size is independent of pad zoom and defaults to one sixteenth when absent or invalid.
- The first version supports forward Session MIDI-Out Clips whose master loop length is evenly divisible by the selected step. Changing Clip direction to Reverse or Ping-Pong, or giving a NoteRow independent length or direction, immediately cuts owned notes, rearms Step and Reset Note or Control Change state, and returns the Clip to pre-step-zero. The Clip then remains visibly unsupported and inactive. Restoring supported timing also remains at pre-step-zero and cannot sound through ordinary Song resume behavior. This safety cleanup never moves, quantizes, or otherwise rewrites stored notes; only the player's requested timing edit keeps its established effect.
- Notes and stepped MIDI Control Change automation execute only on pulse boundaries. An off-grid event waits until the first boundary at or after its stored position and remains unmodified in the project. A note shorter than one pulse lasts one pulse.
- All due note-offs occur before any note-ons at a boundary. When several starts in one NoteRow collapse onto one boundary, only the latest stored start before that boundary emits; the earlier starts remain saved.
- Probability and iterance retain their established decisions at the emitted boundary. Smooth automation, swing between pulses, arpeggiators, MPE timing, and other sub-pulse behavior are not part of this mode.
- A stable-clock watchdog qualifies only after three consecutive intervals from 25 milliseconds through 4 seconds remain within 25 percent of their median. Four missed median intervals cut owned notes and show Waiting without changing phase. Slower or irregular controls continue stepping but do not establish automatic clock-loss timing.
- The next Step after qualified clock loss continues from the frozen phase. A disconnected input clears held-edge state and cuts owned notes only when that exact input has no remaining connection.
- Clip mode, step size, Step binding, and Reset binding survive save, load, and Clip duplication. Transient held-edge, phase, cadence, and pending note-off state never persist or copy.
- Missing or invalid fields recover independently. Missing mode means Song, missing size means one sixteenth, invalid Step keeps External Step visibly inactive, and invalid or missing Reset remains unassigned.
- Assigning or loading the Clip with an internal Synth, Kit, or CV output demotes its Clock mode to Song before normal playback. A valid MIDI-Out assignment retains External Step mode and its saved bindings. No non-MIDI Clip may retain a hidden active External Step mode.
- A Song containing any External Step Clip records the exact local compatibility sentinel `nl-save-schema-1`, even while the feature is Off or its input is unavailable. Schema-1 firmware accepts local schema 0 and 1, rejects future, malformed, or empty local schema sentinels, and otherwise preserves ordinary official and Community version comparison. Older P45 files guarded with `c1.3.1` remain loadable and resave with schema 1. Unsupported firmware refuses the guarded Song instead of silently loading the Clip on the Song clock. The stronger requirement is removed only after every External Step Clip returns to Song mode.

### Compatibility and boundaries

- Preserve ordinary Song-clocked MIDI Clips, their editing, output channel, note and Control Change transmission, launch behavior, probability, iterance, automation, save data, and display behavior.
- Preserve the established meanings and priority of MIDI Learn, global commands, section and Clip commands, Kit-row mute controls, parameter mappings, MIDI Follow, instrument input, recording, and MIDI Thru whenever no active valid External Step binding owns the message.
- Do not apply External Step to Audio, internal Synth, Kit, CV, or Arrangement Clips. Do not add another standard MIDI Clock domain, Song Position Pointer, external Start or Stop ownership, MIDI clock output, linear recording, overdub recording, smooth automation, swing, arpeggiators, sample synchronization, or MPE timing.
- Pulse handling, timeout service, Reset, Stop, and cleanup perform no heap allocation, storage access, sample access, or work in the shared Song next-event scheduler.
- A disabled feature does not consume configured controls. Arrangement playback does not advance Session External Step Clips.
- This deviation is local-only and does not authorize firmware flashing, installation, publication, or upstream submission.

### Verification contract

- Focused checks cover exact device, raw physical channel including MPE-configured ports, type, and number matching; Note and Control Change edge rearming; Program Change edges; independent and shared Step and Reset controls; Reset independence from Step validity; Reset-before-Step ordering; stopped-transport Reset; first step at zero; later steps; loop wrap; off-grid starts; short notes; note-off ordering; collapsed starts; probability; iterance; stepped automation; qualified timeout; irregular clocks; disconnect; feature disable; Step and Reset conflicts; invalid data; and save and load defaults.
- State-transition checks cover conflict entry and exit rearming, one cut on a newly conflicting Step, all configured Clips on Stop and Session exit, non-MIDI output demotion, and unsupported timing edits without an ordinary resume or any extra stored-note rewrite from the safety cleanup.
- Review confirms that every Song tick, resync, launch, live-position, arpeggiator, and shared next-event path excludes External Step Clips; matching controls cannot leak into ordinary MIDI handling; no pulse path allocates or accesses storage; and Stop, Reset, timeout, feature disable, disconnect, conflict transitions, timing edits, and Panic cannot leave an owned note sounding.
- Compatibility checks cover schema 0 and 1 acceptance, future and malformed schema refusal, unchanged ordinary-version comparison, and Songs saved with and without External Step Clips. Current firmware reloads both, an unsupported Community build refuses only the guarded Song, converting every affected Clip to Song removes the stronger compatibility requirement, and an older P45 `c1.3.1` file loads and resaves with schema 1.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical Deluge connected to VCV Rack, check Note, Control Change, and Program Change Step and Reset controls; independent and shared clocks; first step, wrap, Reset, Stop, Panic, clock loss, reconnect, Clip switching, UI feedback, save and reload, and ordinary Song Clips. Physical verification remains human-controlled and does not authorize flashing or installation.

## 24. Release-order-safe preview-only note input

### Intent

Let a player hold the Horizontal encoder while trying notes or drums, hear the ordinary instrument output, and be certain that the complete gesture remains outside the Clip recording. Use the same gesture in Instrument Clip View audition rows and Keyboard View without making its result depend on which control is released first.

### Required behavior

- In Instrument Clip View, holding Horizontal before pressing an audition-column pad marks that note gesture as preview-only for Kit, Synth, MIDI-Out, and CV Clips.
- In every Keyboard View layout that produces instrument notes, including Velocity Drums and melodic, chord, MIDI-Out, and CV layouts, holding Horizontal before a main-grid note press marks every resulting note-on or retrigger as preview-only.
- A preview-only note sounds and releases through the same instrument, MIDI, CV, choke, arpeggiator, effect, velocity, and expression path as ordinary auditioning. It may select the same Drum or note row and show the same note, chord, layout, or recording feedback that does not claim a note was written.
- A preview-only note never records a note-on, count-in early note, retrigger, or note-off into the Clip. It does not create, extend, shorten, or close an existing sequencer note.
- The preview decision is fixed when each note-on or retrigger begins and remains in force until its matching note-off. Releasing Horizontal before the pad cannot create a recorded note-off without a matching recorded note-on.
- A note begun without Horizontal remains an ordinary recording gesture through its matching note-off. Pressing Horizontal after that note begins cannot suppress its required recorded note-off or turn the already sounding note into a preview.
- Repeated notes, overlapping physical pads that produce the same pitch, generated chord notes, encoder-driven note remapping, and Kit retriggers retain balanced sounding and recording lifecycles. Leaving the view, stopping playback, changing Clips or Outputs, or invoking Panic cannot leave a preview note sounding or leave stale preview ownership that affects a later gesture.
- Preview-owned MIDI-Out and CV notes still transmit normally. Only Deluge Clip recording is suppressed.
- When Horizontal is not held as a note begins, audition and Keyboard recording retain their established behavior.

### Compatibility and boundaries

- Preserve Instrument Clip note-grid editing, Horizontal zoom, scroll, note nudge, multiply, row rotation, clipboard shortcuts, and audition-row selection.
- Preserve every Keyboard layout's Horizontal rotation and pressed-Horizontal behavior, including Chord Library voicing, Velocity Drums scroll and zoom, and the separate Scale-modified velocity-profile controls.
- Preserve normal audition silence for an already sequenced row and for established Shift or Vertical modifiers. This deviation does not make a row audible when the existing conflict-avoidance rules require silence.
- Preserve resampling, MIDI Learn, external MIDI input, ordinary live recording, automation, undo history, project data, and firmware compatibility. The preview gesture adds no saved setting.
- Do not change Audio Clip recording, Song or Arranger launch behavior, Slicer audition, Waveform Editor audition, or external-controller note input.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused checks cover preview and ordinary note ownership, both Horizontal and pad release orders, Horizontal pressed after an ordinary note-on, note retriggers, overlapping same-pitch pads, generated chords, encoder-driven remapping, count-in early notes, and cleanup when leaving or stopping.
- Source review confirms that Clip View and Keyboard View decide ownership at note-on or retrigger, pair every sounding and recorded note-off with the correct start, keep preview output audible, and do not disturb Horizontal encoder controls.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical Deluge, check stopped audition, playback, armed recording, and count-in in Kit and melodic Clips; both release orders; Horizontal pressed after a normal note starts; repeated and overlapping notes; chords; held pads while turning Horizontal; MIDI-Out and CV output; view exit; Stop; and Panic. Confirm no stuck sound, stray note, shortened prior note, lost note-off, or changed encoder gesture. Physical verification remains human-controlled and does not authorize flashing.

## 25. Legible full-screen waveform playheads

### Intent

Make the current playback position immediately readable on every full-screen single-source waveform without letting a dense or strongly colored waveform overpower it. Give the OLED sample-editing waveform the same source-specific playback feedback while keeping the display work bounded.

### Required behavior

- Sample Marker Editor, Region and Manual/Lazy Slicer, Audio Clip View, and Sample Browser render the waveform base moderately dimmer than today. The intended level is seven eighths of the established full waveform intensity. Start and End bounds, loop and slice markers, undefined regions, controls, selections, and playhead colors retain their established strength and draw above the waveform.
- A playhead appears only when the audio currently sounding is the exact source represented by the displayed waveform. An unrelated voice, an earlier preview, another Drum, another Clip, or a matching filename without matching sample identity must never drive it.
- Sample Marker Editor follows the newest active voice whose sample holder owns the displayed sample. Audio Clip View follows only the displayed active Clip. Sample Browser follows only its active browser-preview sample. Region and new-Kit Slicer follow only the browser preview, while Manual/Lazy and existing-Kit Slicer follow only the selected Sound.
- The Region Slicer pad cursor spans all eight waveform rows. The Manual/Lazy Slicer cursor spans only its four waveform rows and never covers its slice pads. Other full-screen pad cursors retain the height already owned by their waveform.
- Sample Marker Editor and both Slicer modes add a one-column OLED playhead to their existing waveform companion. It maps the raw source position into the complete visible waveform range, draws after every waveform contour and bound marker, and reverses the pixels in its column so it stays visible over both empty and lit areas.
- The OLED cursor moves only while the exact displayed source is actively playing and its projected display column changes. It disappears after stop, release, source replacement, view exit, or movement outside the visible range. Scrolling or zooming remaps it to the same source position without changing playback or resetting the playhead.
- Moving OLED feedback is refreshed no more than twenty times per second through an existing display service path. An unchanged projected column produces no OLED work. Hiding or replacing a cursor performs one cleanup update so no stale column remains.
- Audio Clip View keeps its existing whole-Clip OLED ruler instead of gaining a second waveform cursor. Sample Browser keeps its existing filename display and gains no OLED waveform.

### Compatibility and boundaries

- Preserve waveform shape, sample analysis, color family, Clip and track color ownership, bounds, slice locations, viewports, scrolling, zooming, reverse playback, audition controls, transport behavior, recording, sample data, project data, and audio output.
- Preserve the established pad cursor color, bound and marker colors, Slicer control colors, grey undefined regions, and every non-waveform pad. Only the full-screen waveform base becomes dimmer.
- Preserve Session, Row, Grid, Arranger, recorder, transition, cached thumbnail, and single-row waveform brightness and cursor behavior. Those multi-source or overview surfaces are not full-screen single-source editors.
- Preserve all seven-segment behavior. Do not add a new display timer, background task, playback engine, persistent setting, saved data, or firmware compatibility requirement.
- OLED drawing consumes only the already cached waveform, marker, viewport, and projected playback state. It performs no sample scan, decoding, cluster load, storage access, memory allocation, audio rendering, or direct display transfer.
- No behavior in this deviation authorizes firmware flashing, installation, publication, or upstream submission.

### Verification contract

- Focused checks cover included and excluded surface selection, the seven-eighths waveform level, untouched overlay strength, exact source matching, newest matching voice selection, Region and Manual row ownership, first and last visible columns, offscreen positions, stopped and replaced sources, reverse movement, loop wrap, scrolling, zooming, and stale-cursor cleanup on exit.
- OLED checks cover one-column mapping, contrast over blank and lit waveform pixels, overlap with selected and unselected markers, visibility transitions, the twenty-hertz limit, unchanged-column suppression, viewport changes, and one final cleanup update.
- Source review confirms that the browser, editor, Slicer, and Clip each use their own authoritative playback source; excluded thumbnails are unchanged; dynamic display work is fixed and bounded; and no display path allocates, accesses storage or samples, renders audio, creates a timer, or sends the OLED directly.
- Format every affected source, compile changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical OLED Deluge, check each included surface with sparse and dense waveforms, forward and reverse playback, loop wrap, start and end edits, Slicer Region and Manual audition, browser preview, markers, popups, scrolling, zooming, stop, source replacement, and exit. Confirm that the waveform is still readable, the playhead is more obvious, no stale cursor remains, and audio, storage streaming, recording, and display response do not regress. Physical verification remains human-controlled and does not authorize flashing.

## 26. Non-destructive Song compatibility preflight

### Intent

Refuse a Song that requires unsupported firmware or a newer local save schema before loading can stop playback, change the interface, clear Undo history, replace the current Song, or otherwise mutate live project state.

### Required behavior

- Every XML and JSON Song load performs an initial compatibility-only read before any destructive load action. The file is closed after that read and reopened from the beginning only when it is compatible.
- The compatibility read recognizes the established official and Community firmware fields and the exact local save-schema sentinel stored in the existing compatibility field. Local schema 0, 1, and 2 are supported. A future schema, malformed local sentinel, or empty local sentinel is rejected.
- Compatibility fields may appear anywhere in the Song root. Unknown fields before them, including nested arrays or objects and strings containing braces, brackets, escaped quotes, or escaped backslashes, cannot hide a later incompatibility marker or corrupt the second read.
- Canonical Songs with leading compatibility fields stop the initial read as soon as the complete compatibility requirement is known. Legacy Songs without a compatibility field remain loadable after the root has been checked.
- An empty ordinary firmware-version value is treated as the established unknown version instead of causing an invalid memory access. Other invalid ordinary version strings retain their established unknown-version meaning.
- An incompatible Song preserves the exact incompatibility error and leaves playback, the current Song, the active interface, Undo and Redo history, and all loaded project state unchanged.
- A read, close, reopen, malformed-file, or storage-removal failure preserves its own error. Each successful file open has one matching close, and a failed reopen cannot reuse stale parser state or an earlier file handle.
- The normal load performs its established defensive compatibility checks again. A compatibility error found there is propagated rather than discarded.

### Compatibility and boundaries

- Preserve compatible XML and JSON Song loading, legacy Song loading, ordinary official and Community version comparison, browser behavior, file selection, error reporting, and the complete normal deserialization path after reopen.
- Kit presets and saved-row presets continue using their established compatibility reads and error propagation. This deviation does not add a second preflight to those non-Song files.
- Do not rewrite, repair, migrate, resave, or otherwise modify a refused file. Do not change Song contents, project formats, playback semantics, Undo semantics, or storage ownership beyond the compatibility-only first read and required reopen.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Production XML and JSON reader checks cover canonical and reordered compatible headers, too-new ordinary versions, local schemas 0 through 2, future schemas, malformed and empty sentinels, legacy files without markers, empty and invalid ordinary versions, malformed input, nested unknown payloads with structural characters inside escaped strings, and storage failure between the two opens.
- State-order checks prove that refusal occurs before playback, interface, Undo, Redo, and current-Song mutation; compatible files close and reopen exactly once; and the normal reader still propagates a defensive incompatibility result.
- Format every affected source, compile the changed storage and Song-loading units with the target ARM Release toolchain, run the complete configured host suite, and complete a local Release firmware build.
- A physical Deluge check loads representative compatible, legacy, unsupported, malformed, and storage-interrupted Songs and confirms that every refusal leaves the playing project and Undo history intact. Physical verification remains human-controlled and does not authorize flashing or installation.

## 27. Kit Sound Drum choke groups

### Intent

Let one Kit contain several independent sets of mutually exclusive samples while keeping the existing Polyphony control as the one clear switch for whether a row participates in choking.

### Required behavior

- Every audio Sound Drum owns one choke-group value from 1 through 16. Missing, zero, or otherwise invalid saved values normalize to group 1. New Sound Drums and newly sliced rows begin with group 1 and their established non-CHOKE Polyphony default.
- Polyphony `CHOKE` is the only participation switch. A Sound Drum retains its saved group while Polyphony is another mode, but it neither chokes nor is choked until Polyphony returns to `CHOKE`.
- Starting an ordinary audio Sound Drum releases every currently sounding `CHOKE` Sound Drum in the same Kit and same group before the replacement starts. This includes the triggering row's previous voice, so self-retriggers remain click-safe.
- Ordinary sequencer notes, held row audition, incoming Kit MIDI notes, and Cut, Once, Loop, and Stretch playback all use the same group behavior. Different groups, non-CHOKE Sound Drums, MIDI rows, and Gate rows never release one another.
- A normal trigger applies the group release only when it resolves to an immediate Sound Drum start. Direct starts, Kit-arpeggiator bypass, one-shot fallback, and an immediate Kit-arpeggiator output release the group exactly once immediately before starting. Duplicate, deferred, probability-suppressed, or otherwise no-output Kit-arpeggiator inputs do not release the group. Later starts generated internally by that arpeggiator do not rescan or mutate Kit rows.
- Releasing a matching row stops only MIDI notes currently tracked as belonging to that row, resets that row's arpeggiator and inversion state so it cannot retrigger later, fast-releases its active audio, and updates its render eligibility before the replacement begins. It never sends channel-wide All Notes Off. Delay and reverb tails continue through the established fast-release behavior.
- The dedicated held-Select Kit Waveform Editor preview from deviation 19 does not trigger group release and is protected from unrelated group release while that dedicated preview owns the row. Normal held row audition remains a participating ordinary trigger.
- Group identity follows the Sound Drum through Song save and load, Kit presets, saved-row presets, full Sound Drum copy and paste, row reorder, and deletion. Notes-only operations do not invent or change group identity. Deleting a row destroys its group with that Sound Drum; reordering requires no group-specific remapping.
- Replacing an existing Slicer anchor Sound Drum preserves that Sound Drum's group and Polyphony according to the established in-place replacement behavior. Newly appended Slicer rows use group 1 and the ordinary non-CHOKE default.
- A stored group 2 through 16 requires local save schema 2 even when that Sound Drum is not currently `CHOKE`. Group 1 alone adds no new requirement. External Step alone requires schema 1; a file containing both records the maximum requirement, schema 2.
- Songs, Kit presets, and saved-row presets write a non-default group value and the appropriate compatibility marker. Schema-2 firmware accepts schemas 0, 1, and 2 and rejects future, malformed, or empty local schema sentinels. An unsupported build must refuse protected data rather than silently collapsing every row into group 1.
- With a selected Kit Sound Drum set to Polyphony `CHOKE`, pressing Select on that value opens a group selector from 1 through 16. OLED shows `Choke group` and values `1` through `16`; seven-segment displays show `CHGP` and `G01` through `G16`. Other Polyphony values, Synths, MIDI rows, Gate rows, and non-Kit contexts do not expose the selector.

### Compatibility and boundaries

- Preserve the established Polyphony meanings, ordinary Kit triggering, row and Kit arpeggiators, sample playback modes, MIDI and Gate behavior, effects tails, audition selection, Sound Drum clipboard behavior, row reorder and deletion, Slicer placement, and project data unrelated to the new group field.
- Do not add a pad shortcut, another Polyphony value, a second enable switch, a bulk group editor, a horizontal menu slot, group lighting, MIDI or Gate groups, channel-wide All Notes Off, global effects cancellation, or render-time Kit-list traversal.
- Keep group scanning at the ordinary resolved Sound Drum start boundary. It must not allocate memory, access storage, change Kit row membership, or run from an audio render or arpeggiator tick callback.
- No local behavior authorizes flashing, installation, publication, or upstream submission.

### Verification contract

- Focused checks cover group normalization, group membership, self-retrigger, independent groups, inactive saved groups, exact tracked MIDI note-off cleanup, row-arpeggiator reset, ordinary-start versus internal-arpeggiator ownership, copy and clone behavior, persistence defaults, schema aggregation, menu visibility, and OLED and seven-segment formatting.
- Persistence checks cover Songs, Kit presets, saved-row presets, missing and invalid group values, group 1 without a stronger marker, groups 2 through 16 with schema 2 even while non-CHOKE, External Step alone with schema 1, combined features with schema 2, and refusal of unsupported or malformed schemas.
- Source review confirms that reorder and deletion rely on existing Sound Drum identity, Slicer anchors preserve existing state, new Slicer rows use defaults, direct Waveform Editor preview remains protected, no channel-wide MIDI message is sent, and no cross-row scan occurs from render or tick callbacks.
- Format every affected source and localization input, regenerate localization through the established generator, compile the changed production units with the target ARM Release toolchain, run the complete configured host suite, and complete one uninterrupted logged local Release firmware build.
- On a physical OLED and seven-segment Deluge, check groups 1 and 2 in one Kit, self-retrigger, held audition, sequencer and incoming MIDI triggers, Cut, Once, Loop, Stretch, row and Kit arpeggiators, external MIDI echo shared with unrelated rows, delay and reverb tails, the dedicated Waveform Editor preview, copy and paste, reorder, deletion, Slicer anchor replacement, new slices, menu navigation, save and reload, schema refusal on older firmware, and malformed-file refusal. Physical verification remains human-controlled and does not authorize flashing or installation.

## Maintaining this document

Each new local deviation adds or updates a contract in this file in the newest local feature revision. A completed document must remain understandable without local patch history, owner plans, or conversation context. It must identify the affected surface, exact required outcome, behavior that must remain stable, meaningful edge cases, and the evidence needed to verify the deviation. Do not add code, pseudocode, or implementation recipes.
