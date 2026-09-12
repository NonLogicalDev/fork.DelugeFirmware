# Keybinding catalog

`bindings.jsonl` contains 1,273 source-reviewed binding records from the full source inventory and subsequent shortcut changes in this local firmware checkout. The coverage ledger accounts for all 391 definitions discovered by the input-handler scanner, plus manually reviewed helpers and mappings: 518 ledger rows in total. The audit tracks 184 source files across bindings, control definitions and the ledger. This is source documentation, not a hardware-tested guarantee that every possible dispatch path has been found. Firmware source remains authoritative for runtime behavior.

Coverage includes global controls, Session Rows/Grid, Arranger, Audio/Instrument Clips, Automation, Performance, keyboard layouts and sidebar controls, sample editors and slicers, browsers/load/save/rename, Sound Editor, context menus, parameter-pad tables, DX7, gold-knob behavior and MIDI-learning gestures. Configurable parameter/assignment families are represented as families; fixed Sound Editor and DX7 pad targets have individual entries. Disabled legacy hardware branches and no-op/forwarding handlers have explicit coverage explanations. Arbitrary user-learned MIDI assignments are not a finite list of physical shortcuts.

Write one complete JSON object per line, without comments, blank records, or an enclosing array. Add each reviewed binding as it is discovered; do not wait for a complete action inventory. Keep existing record order during small edits so a review shows the actual additions. The format is draft 2; the seven initial draft-1 entries have been migrated in place.

## Record fields

| Field | Meaning |
| --- | --- |
| `id` | Stable binding identity. Do not encode line numbers or revision hashes. |
| `context` | Structured `view`, `layout`, `output`, and/or `dispatch` context. Each supplied field must match. `dispatch` identifies a reached handler layer, including inherited Browser/QWERTY/Save/Rename handlers; it is not a literal screen name. |
| `key` | An event string abbreviates a trigger. A gesture object has one `trigger` and optional `hold` controls. An ordered `sequence` contains gesture objects or shorthand strings. Rotation is a logical key such as `horizontal.turn`, used through `trigger`. |
| `command`, `args` | Semantic action and its arguments. Several bindings may invoke the same command with different arguments. |
| `title`, `description` | Search result and instructions for a person using the Deluge. |
| `when` | Pseudocode condition after mode matching. Named predicates are explained below; no expression evaluator is implemented yet. |
| `requires` | Conditions for success after a gesture matches, including consumed-error outcomes. |
| `tags`, `keywords` | Controlled filter categories and free-text synonyms respectively. |
| `dispatch`, `lifecycle` | Handler stage, consumption, forwarding, deferred processing, ownership, release, cancellation, and partial effects. |
| `feedback` | Relevant display/audio/pad response; localization identifiers may be used as evidence. |
| `sources` | Repository-relative file, C++ symbol, role, and optional branch anchor. Qualified symbols may be declared inside namespace blocks. |
| `availability` | Firmware snapshot/profile being described. `local` means observed in this checkout, not proof that the binding was introduced locally. |
| `verification` | Source review, review date, device evidence, scope limits, and unresolved findings. |

IDs, commands, contexts, control names, and predicate names are catalog vocabulary, not necessarily C++ identifiers. `context` is implicitly ANDed with `when`. Required `hold` controls allow other controls unless an explicit condition excludes them. `shift` in `hold` uses effective Shift (including sticky behavior); physical pad holds and encoder push switches use their physical state, subject to documented ownership latches. Absence of Shift is expressed as `!shift.active` in `when`.

`controls.json` defines 49 physical controls, control families and semantic pad roles, with aliases and source-backed coordinate resolvers. Canonical names include `clip-view`, `session-view`, `gold[index]` (0 lower, 1 upper), `mod-bank[index]` (zero-based), and `grid[x,y]`. `row` binds one displayed row. Roles such as `audition[row]`, `slice[index]` and `marker[bound]` depend on the current view and state. Firmware button-scan coordinates are separate from pad coordinates and must not be used as front-panel drawing coordinates.

An encoder push switch and its rotation are different logical keys. Rotation keys carry signed detents as their input value; `samplesPerDetent` or `semitonesPerDetent` multiplies that value. They do not imply a physical button press or a later release event. Only switch/pad keys support holding and releasing. Trigger names explicitly identify the event: `learn.down`, `horizontal.up`, or `horizontal.turn`. Held states use control names without an event suffix. Delayed holds currently arm on `.down`; their timer and cancellation behavior lives in `lifecycle`, not a separate timer-event matcher. The few incoming MIDI-learning records describe message eligibility in `when`; their suffixes are not physical switch edges.

For example, `{"hold":["horizontal"],"trigger":"horizontal.turn"}` means rotate while holding the encoder switch. `{"sequence":[{"trigger":"shift.down"},{"trigger":"shift.down"},{"trigger":"shift.down"},{"trigger":"shift.down"},{"trigger":"shift.down"}],"maxGapMs":500,"inclusive":true}` describes Panic. Timing measures adjacent eligible trigger events, not the entire sequence. Switch releases between presses are implicit and do not themselves advance the sequence. Cancellation remains explicit. Repeated-copy entries retain their immediate first-press side effects and same-row requirement in `when` and `lifecycle`.

Current tags: `transport`, `recording`, `looping`, `clipboard`, `navigation`, `editing`, `sampling`, `slicing`, `automation`, `midi`, `sound-design`, and `system`. Add useful shared categories deliberately. Keywords can include phrases such as “duplicate drum,” “lazy chop,” or “stop feedback”; they do not participate in gesture matching.

## Predicate meanings and evidence

| Predicate | Meaning and source |
| --- | --- |
| `shift.active` | Effective `Buttons::isShiftButtonPressed()` state. |
| `scaleMenu.entryEligible` | `ScaleMenu::canOpen`: a Song exists, current UI is the root, UI mode is NONE, effective Shift is active, no matrix pad or physical button other than Shift/Scale is held, and root is Session, Arranger, Performance, Arranger Automation, or a melodic Instrument Clip/Keyboard/Clip Automation view. Existing overlays, Kit clip views and Audio Clip view are excluded. |
| `scaleMenu.ownsRelease` | `ScaleMenuGesture::ownsRelease_`, armed when opening is attempted and cleared by the initiating Scale release even after closing the menu. |
| `scaleMenu.editing`, `scaleMenu.row` | `ScaleMenu::editing_` and `row_`; row 0 is Root and row 1 is Mode. These select field navigation versus immediate value editing. |
| `slicer.currentSliceValid` | `currentSlice < numManualSlice` in the Vertical press branch. |
| `output.kind` | `getCurrentOutputType()` in `InstrumentClipView::buttonAction`. |
| `midiLearn.active` | `isUIModeActive(UI_MODE_MIDI_LEARN)` in that handler. |
| `audition.oneRow` | `oneNoteAuditioning()` in that handler. |
| `audition.rowIsSoundDrum` | The displayed audition row exists, has a Drum, and its type is `DrumType::SOUND`. |
| `ui.mode` | `currentUIMode`; `adding-drum-noterow` means `UI_MODE_ADDING_DRUM_NOTEROW`. |
| `kitCopy.repeatEligible` | Nonzero previous copy timestamp, same `lastAuditionedYDisplay`, and unsigned sample-timer delta strictly less than `kShortPressTime`. The copy handler arms or resets this state before attempting the clone. |
| `kitCopy.clipboardMatchesSongAndKit` | A copied Drum exists and its recorded Song and Kit match the current ones in `pasteKitRow`. |
| `kitCopy.destinationAvailable` | A permitted new destination or existing row whose Drum is absent or is a Sound Drum, as checked by `pasteKitRow`. |
| `copy.allocationAndCloneSucceeded`, `paste.allocationAndCloneSucceeded` | The relevant allocation and clone operations succeeded; their failures are consumed errors, not permission for another binding to run. |
| `slicer.mode` | `slicerMode`; `manual` means `SLICER_MODE_MANUAL`. |
| `slicer.horizontalPressOwned` | Stored `horizontalEncoderPressed` latch in Slicer, not a new read of raw hardware state. |
| `slicer.horizontalPressUsed` | Stored `horizontalEncoderPressUsed` cancellation latch. |
| `slicer.usesExistingKit` | Slicer's `usesExistingKit` flag. |
| `dispatch.shiftReachedGlobalFallback` | The current UI did not consume or defer this Shift event before the Panic counter in `Buttons::buttonAction`. |

These definitions support review; they are not a complete condition solver. Trace the entire forwarding path before claiming that a binding is reachable in every mode or that two conditions cannot overlap.

The Scale menu forwards pads and Vertical rotation to the originating view's native handlers, including their modifiers, actions and no-op cases. Its delegation records are not replacement bindings for every underlying action. Select and Back still navigate the menu. A native pad gesture may open another UI; the Scale menu must not overwrite that UI's display. Normal note and release handling stays with the originating mode, without a separate menu-owned note lifecycle.

## Incremental maintenance

1. Append a record after checking its input handler and relevant helpers. Split different triggers/arguments into separate IDs and share the semantic command where appropriate.
2. Record failure, release, and cancellation behavior when relevant. Keep unknowns explicit in `verification.scope`.
3. Parse every line, check unique IDs, and verify source anchors before checkpointing. Source review does not establish physical behavior or exhaustive coverage.
4. Update the affected record when its handler changes. Preserve observed inconsistencies in a finding instead of silently documenting the preferred behavior.
5. Generate search data and audit reports from this file. The local manual must not become a separate hand-edited copy.

## Checks and audit files

Run from the repository root:

```sh
uv run docs/keybindings/validate.py
uv run docs/keybindings/audit.py
uv run docs/keybindings/test_catalog.py
```

- `validate.py` checks JSON Schema, unique IDs, registered control families/events, and source file, symbol-token and anchor existence. Token checks do not establish symbol ownership or predicate correctness.
- `coverage.jsonl` records reviewed handlers, their binding IDs, exclusions, reasons and file hashes. `inventory.py` discovers likely input definitions, including inline headers; it is a name-based scanner, not a C++ parser. Overloads in one file reconcile by method name, so ledger review is still necessary.
- `source-snapshot.json` records hashes of all binding and control-registry source references. `audit.py` fails for uncovered scanner candidates, changed files, unresolved ledger gaps, or unknown binding references. Refresh hashes only after reviewing the source changes; replacing hashes alone is not a review.
- `audit.py --json` lists 193 same-context/gesture groups for follow-up. These are potential overlaps, not 193 bugs: conditions and dispatch precedence often distinguish them. It does not yet resolve inherited contexts, overlapping pad roles, or arbitrary pseudocode conditions.
- `findings.jsonl` records specific source observations separately from desired firmware behavior. No fixes are implied.
- `test_catalog.py` tests valid/invalid gestures, duplicate IDs, missing coverage, source drift (including helper-only references), unresolved gaps and gesture hold-order normalization.
- `merge.py` emits an `apply_patch` patch for incremental worker-shard integration; it never writes or silently removes records. Reconcile aliases and semantics after merging.

The searchable local page and executable context/condition resolver remain pending. Catalog-only changes do not require a firmware rebuild; new firmware shortcuts still require their own build and device validation.
