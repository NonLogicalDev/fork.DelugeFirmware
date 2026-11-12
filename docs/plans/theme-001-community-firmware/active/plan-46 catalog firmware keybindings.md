---
description: "P50: define a source-backed Deluge shortcut catalog for consistency audits and fast searchable lookup."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Catalog firmware keybindings

## Purpose

Describe every firmware control binding in one systematic representation that supports code audits, inconsistency reports, and a searchable local manual. The operator requested a VS Code-like format, including modes, keys, actions, descriptions, conditions, source files, tags, and keywords on [[2026-09-06]].

## Current State

- Full-inventory workers completed: `/root/catalog_views` reviewed timeline, Session/Arranger, Clip, Automation, Performance and Clip Minder controls; `/root/catalog_editors` reviewed keyboard layouts, sample editors, browsers/load/save/rename/QWERTY and control identities; `/root/keybinding_evidence` reviewed Sound Editor, menus and shortcut tables. `/root` integrated and validated the canonical catalog, HID/global playback, target-specific gold controls and coverage.

- Latest scope: catalog ALL firmware shortcuts. Inventory global dispatch, view handlers, inherited controls, inline handlers, keyboard layouts, menus, browser/load/save surfaces, and configurable control families. Reconcile each input-producing branch to a record or explicit exclusion; keep records incremental and never claim completeness from a raw handler count.

- Status: first full source inventory completed; 1,261 canonical JSONL records, 508 coverage rows, all 384 scanner-discovered definitions accounted for, and 180 source files tracked. Twelve focused regression tests pass. Hardware behavior and whole-program reachability are not verified.
- The accepted gesture fields are `hold`, `trigger`, and `sequence`; rotation is a separate logical input such as `horizontal.turn`. The control registry has 49 definitions and 9 alias rules. Browser/QWERTY/Save/Rename inheritance is represented as dispatch context, not an exact leaf-screen match.
- Eight source-backed findings are recorded separately. The overlap report lists 190 same-context/gesture groups for review, not 190 proven conflicts. Predicates, inherited dispatch, delayed-hold matching and overlapping pad-role resolution still need executable semantics before claiming complete conflict detection.
- Next phase: searchable local UI and deeper conflict/reachability review. No site or firmware behavior changed in this catalog pass.

## Scope

### In scope

- A machine-readable catalog format with stable binding and action identities, modes, physical gestures, conditions, tags, keywords, source anchors, and verification state.
- A repeatable audit for missing bindings, overlapping gestures, inconsistent actions, event consumption, and source drift.
- A searchable local UI for forgotten shortcuts, including ordinary-language synonyms and mode/control filters.
- Both upstream and local bindings, including ordinary single controls, pad regions, encoders, held combinations, release actions, repeated taps, menu shortcuts, and configurable physical assignments.

### Out of scope

- Changing firmware behavior, making shortcuts user-remappable, rewriting runtime dispatch, fixing discovered inconsistencies without a separate accepted fix, flashing, or publication.
- Treating dynamic MIDI assignments as fixed hardware shortcuts; record their learning gestures and configurable action families with an explicit distinction.

## Product integration

The firmware distributes controls across global dispatch, views, inherited handlers, menus, and layouts. A catalog beside the source should feed both technical audits and a task-focused lookup page in the existing local manual. The lookup page shows what to press and when; code details remain expandable. Machine-readable gestures and shared context definitions prevent display labels and audit logic from becoming competing descriptions.

## Completion Criteria

- [x] The format covers representative press, release, turn, pad, repeated-tap, and inherited-handler cases with source evidence.
- [x] The schema and catalog validate through a documented entrypoint.
- [x] The source-inventory pass maps reviewed input handlers and helpers to records or explicit exclusions. All scanner candidates reconcile; this does not prove the scanner discovers every possible branch or input mechanism.
- [x] The audit reports source drift and potential gesture overlaps without claiming that arbitrary conditions prove disjointness.
- [ ] A local searchable page supports task words, aliases, mode, control, tags, and verification filters, with stable result/detail geometry.

## Blockers

None.

## Artifacts

- D1 `../../../_owner/docs/Deluge keybinding catalog.md`: design, taxonomy, evidence and pending UI behavior.
- R1 `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/docs/keybindings/`: canonical JSONL, schema, controls, coverage, source snapshot, findings, validator and audit/tests; local revision `nl/patch-keybindings-01`.
- U1 `/Users/nonlogical/Projects/local/active/app.deluge-synth-architecture`: intended local lookup consumer; no changes in this design pass.

## Decisions

- Accepted steering supersedes the earlier separate press/release/turn fields: use `hold`, `trigger` and ordered `sequence`. Trigger event suffixes distinguish down, up and turn. Keep JSON constructs, structured context and additional pseudocode conditions in `when`.

- Keep the VS Code concepts `key`, `command`, `args`, and `when`; extend the gesture representation for physical pads and encoders. Use JSONL with one complete JSON object per line, no comments or enclosing array. Write reviewed entries incrementally; the catalog starts partial.
- Separate controlled tags from free-text keywords. Tags drive consistent filters; keywords match the operator's vocabulary.
- Use repository-relative file plus symbol anchors. Current line links are generated at the audited revision.
- Keep observed behavior separate from desired behavior and audit findings. Unknowns remain searchable and visibly unverified.
- Keep the firmware source authoritative for runtime behavior; the catalog is its audited documentation, with generated views rather than a second manually maintained shortcut list.

## Implementation Steps

- [x] Establish the representation and verify difficult examples against current source.
- [x] Add catalog, schema, validator, coverage audit, tests and developer-workflow deviation in the dedicated local Jujutsu change.
- [x] Inventory global routing and each view/layout/menu family; reconcile scanner candidates and manually found helpers, including exclusions.
- [ ] Review every overlap candidate with full dispatch conditions; eight specific findings have been recorded, not fixed.
- [ ] Add the local lookup UI, export/sync workflow, and representative search checks.

## Work Log

- [x] [[2026-09-06]] - Expanded the canonical file incrementally from 11 to 1,261 entries. Reconciled the full scanner inventory, normalized physical aliases, documented dynamic pad roles, added source-drift checks and twelve tests, and fixed catalog precision defects found by independent review. No firmware, build, site, flashing or publication changes.

- [x] [[2026-09-06]]@14:39 - Verified the recorded owner matches the active task, selected the next available plan number, and started a read-only source audit. The operator added tags, keywords, consistency review, and searchable lookup to the purpose. No firmware or site file has changed.

## Validation

- Current catalog: all 1,261 entries pass JSON Schema, unique-ID, registered control-family/event and textual source-reference checks. The audit reports zero unaccounted scanner candidates, changed source files or invalid coverage references. Twelve regression tests pass, including accepted/rejected gestures, unknown controls, unsupported events, duplicate IDs, uncovered handlers, unresolved gaps and helper-only source drift. These checks do not prove arbitrary predicate correctness, exhaustive discovery or hardware behavior.

- Owner check: `README.md` and `CODEX_THREAD_ID` both identify `01a009d3-b4ef-7372-9753-e980d3eda5d5`.
- Reference check: the official VS Code keyboard-shortcut documentation describes `key`, `command`, `when`, and optional arguments; the Deluge format will extend those concepts for its control hardware.
- Source-inventory review: completed within the declared scanner/manual-review limits. Search UI and device checks remain pending.

## Unfinished Work

- [x] Migrated the initial seven entries to `context` and JSON-native gestures; appended four Slicer bindings. The accepted event vocabulary uses `trigger` with `.down`, `.up`, or `.turn`, and `hold` uses control names without suffixes.

- [x] Finalize this source-catalog pass, control identities, coverage audit and validation.
- [ ] Implement the searchable local UI and executable context/condition resolution; review physical behavior and open findings separately.
