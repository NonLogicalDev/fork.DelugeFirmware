---
description: "Add independent, persistent choke groups to audio Sound Drum rows in Deluge Kits."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Add Kit choke groups

## Purpose

Let one Kit contain several independent groups of mutually exclusive samples without adding another pad shortcut or changing non-audio row semantics. This plan began on [[2026-08-30]].

## Current State

- Status: software-complete; physical Deluge behavior and real-device save/reload remain.
- Result: `nl/patch-kit-rows-02` is Jujutsu change `lrnvyroynrow` at commit `a09b3012`. Audio Sound Drums have 16 persistent CHOKE groups; release occurs only at resolved ordinary starts, sends exact tracked row MIDI note-offs without CC123, resets row-arp ownership, fast-releases audio, and preserves effects tails. Schema 2 protects non-default groups. Both final independent reviews are clean.
- Next action: physically validate independent groups, held audition, arpeggiators, MIDI note-off balance, displays, effects tails, and Song/Kit/row save-reload before device behavior is claimed.

## Product integration

- **Existing model:** `Polyphony = CHOKE` makes audio Sound Drums participate in one Kit-wide click-safe choke pool.
- **User intent:** use several independent hi-hat, percussion, vocal, or loop choke sets in one Kit without sacrificing the existing Polyphony control.
- **Integrated model:** Polyphony remains the participation switch, each `SoundDrum` owns one saved group number, and `Kit` owns cross-row release. Group assignment is configured beneath CHOKE rather than through a new performance shortcut.
- **Architecture impact:** make the existing Kit-wide release group-aware; add one defaulted row property, guarded persistence, one contextual menu item, localization, and lifecycle tests. Do not introduce a second choke engine or mutate Drum lists from render callbacks.

## Scope

### In scope

- Choke groups 1 through 16 for audio `SoundDrum` rows only.
- Existing `Polyphony = CHOKE` as the only runtime membership switch.
- Selected-row menu path `Sound → Voice → Polyphony → CHOKE → Choke group` using the established menu model where possible.
- OLED label and values `Choke group`, `1` through `16`; 7SEG label `CHGP` and values `G01` through `G16`.
- Same-group click-safe fast release on every ordinary row trigger, including self-retrigger and held audition; different groups remain independent.
- Song, Kit, saved-row preset, Kit-row clipboard, reorder, deletion, and Slicer creation/replacement behavior.
- Backward-compatible group 1 and an older-firmware refusal marker whenever saved behavior depends on groups 2 through 16.
- A pre-load Song compatibility check that refuses unsupported save schemas before changing playback, UI, undo, or the current Song.

### Out of scope

- MIDI or Gate rows, a modal member editor, audition-column group lighting, pad shortcuts, global effects-tail cancellation, a second Polyphony setting, firmware flashing, installation, publication, or upstream submission.

## Behavior contract

- A `SoundDrum` participates only while its Polyphony mode is CHOKE. Switching away disables its participation; its saved group selection remains row state so returning to CHOKE restores the configured group.
- Missing group data means group 1. Existing Kits with several CHOKE rows therefore retain their current one-pool behavior.
- Triggering an audio `SoundDrum` fast-releases every sounding audio `SoundDrum` in the same Kit whose Polyphony is CHOKE and whose group matches, including the triggering row's previous voice. The new voice starts through the ordinary note-on path.
- Audition, sequencer, MIDI-triggered Kit notes, retrigger, Cut, Once, Loop, and Stretch all use the same group behavior. Delay and reverb tails continue through the existing click-safe release semantics.
- Cross-row release occurs at the Kit's ordinary resolved Sound Drum start boundary, before the replacement voice is started. A normal trigger that immediately produces a Kit-arpeggiator output applies the group once. Later starts generated from Kit arpeggiator render or tick callbacks never scan or mutate the Kit Drum list and do not initiate another cross-row choke.
- Different groups never release one another. Non-CHOKE Sound Drums and all MIDI or Gate rows remain unaffected.
- Group identity follows the Sound Drum through save/load, presets, row copy/paste, and reorder. Deleting a row destroys its group with that Sound Drum; a replacement row owns its own copied, loaded, or default group. Copying a Sound Drum copies its group; copying notes only does not invent or change group state.
- A newly created or sliced row uses the ordinary non-CHOKE default and group 1. Replacing an existing anchor Sound Drum preserves that anchor's current group and Polyphony unless the established replacement contract explicitly replaces the whole Sound Drum.
- Group 1 adds no compatibility requirement beyond existing CHOKE behavior. Any saved row with group 2 through 16 records local save schema 2 in the existing compatibility field, even while its Polyphony is not CHOKE. External Step alone records schema 1; a file needing both records the maximum schema 2. Unsupported, future, malformed, or empty local schema sentinels fail closed. Ordinary official and Community version markers retain their established comparison.
- Local save-schema sentinels remain stable across upstream numeric version changes. Unpatched firmware treats them as unsupported, so future upstream builds without the capability cannot silently accept and collapse grouped rows.
- Song compatibility is checked from a read-only XML or JSON root-header preflight before any playback, UI, undo-log, or current-Song mutation, then the file is reopened for normal loading. Kit and saved-row preset readers continue to propagate the same incompatibility result.
- Normal held row audition participates in its group. The dedicated held-Select Waveform Editor preview remains protected under deviation 19 and does not choke or get choked by unrelated Kit rows.
- Choking a Sound Drum sends note-offs only for that row's tracked post-arpeggiator MIDI notes, resets its row-arpeggiator ownership, performs the established fast release, and reassesses render skipping before the replacement starts. It never sends channel-wide MIDI All Notes Off, so unrelated rows or tracks sharing an external channel remain sounding.

## Completion Criteria

- [x] Groups 1 through 16 have independent software paths for audio Sound Drums, including self-retrigger and held audition.
- [x] Existing missing-field Kits behave exactly as group 1 and require no stronger compatibility marker.
- [x] Group 2 through 16 data follows the supported save/copy/reorder paths and records fail-closed local save schema 2; External Step uses schema 1; unsupported readers refuse before Song state changes.
- [x] The contextual OLED and 7SEG menu is reachable only for the selected audio Sound Drum's CHOKE mode and has no new pad shortcut.
- [x] Slicer-created rows default inactive/group 1, while a replaced anchor preserves the behavior required by its replacement path.
- [x] Focused runtime, persistence, compatibility, menu, and lifecycle tests pass with adversarial review.
- [x] Formatting, affected ARM objects, the complete configured host suite, and one uninterrupted logged Release build pass.
- [ ] Physical Deluge checks confirm click-safe behavior, held-pad release, independent groups, menu usability, save/reload, and effects tails before device behavior is claimed.

## Blockers

- None for implementation. The pre-existing destructive Song compatibility-load defect is an explicit dependency revision, not an unresolved design question.

## Artifacts

- **A1: P48 plan.** This file owns behavior, scope, design decisions, validation, and recovery state.
- **A2: Kit choke-group feature.** Complete in Jujutsu change `lrnvyroynrow`, commit `a09b3012`, bookmark `nl/patch-kit-rows-02`. Eight production-linked P48 cases, four focused targets, all 23 configured host targets, formatting, 13 affected ARM Release objects, two final independent reviews, and the 492-step Release build passed.
- **A3: Local behavior contracts.** Complete in repository `DEVIATIONS.md`: corrected deviation 23, safe Song preflight deviation 26, and self-contained Kit choke-group deviation 27.
- **A4: Repository guidance.** Complete in checkout-local `AGENTS.local.md` for runtime, persistence, UI, and compatibility boundaries.
- **A5: Song compatibility preflight.** Complete in Jujutsu change `wxsvvosv`, commit `da03c661`, bookmark `ns/bug-song-load-01`, positioned before local saved-schema features. Final focused tests passed 16 cases, configured host tests passed 16 of 16, formatting passed, five affected ARM Release objects compiled, and independent review found no P1 or P2 issue.
- **A6: External Step compatibility correction.** Complete in existing Jujutsu change `plloqspk`, commit `61315dd4`, bookmark `nl/patch-midi-sequencer-01`; schema 1 focused and host validation passed, and independent review found no P1 or P2 issue.
- **A7: Final Release evidence.** Build log `/private/tmp/deluge-p48-kit-choke-groups-release-20260831.log`; 492 of 492 steps passed from 2026-08-31T07:22:08Z to 07:29:20Z in 432.26 seconds. The only warnings were the existing ignored-fallthrough attribute and RWX load segment warnings.
	- `deluge.bin`: 1,740,768 bytes; SHA-256 `90ba2952e18e4b72a3b284ee0b1d2ad5600b3b2c6a3278ba46e0a62d4b1d19d1`.
	- `deluge.elf`: 65,498,960 bytes; SHA-256 `e17b3f716b380f92bf45d0886f5cb333f79e9338f5bbec2fb4f2de686b3d99f3`.
	- `deluge.hex`: 4,896,321 bytes; SHA-256 `2b0c8ed39c141646faa3b040dadcbbcf58714822e74a112b3872d70a62b67eac`.
	- `deluge.map`: 21,254,068 bytes; SHA-256 `30cb9da00391dc467c91b9881808b7dcfb35991868b13674ab01586143fd2006`.
	- `deluge.nmdump`: 752,600 bytes; SHA-256 `c380988f42ce3739e40dd6e87ad69922b54908e9331458e3e401f49d29ce29e0`.

## Decisions

- **Decision:** extend `Polyphony = CHOKE` instead of adding another enable control.
	- **Rationale:** one row setting should answer both whether choking is active and which pool owns it.
- **Decision:** store group identity on `SoundDrum` and let `Kit` own cross-row release.
	- **Rationale:** membership belongs to the row, while safe iteration and same-Kit scope belong to the Kit aggregate.
- **Decision:** retain the selected group while Polyphony is not CHOKE, but make it behaviorally inactive.
	- **Rationale:** changing voice allocation should not destructively erase configuration; compatibility still follows any saved non-default group.
- **Decision:** defer bulk member editing.
	- **Rationale:** it needs a separate modal pad-routing model and is not required for a maintainable v1.
- **Decision:** use versioned local save-schema sentinels instead of bumping the numeric firmware identity.
	- **Rationale:** a numeric bump protects only until upstream reaches the same version. An exact local schema marker fails closed in unpatched and future upstream firmware and remains stable across rebases.
- **Decision:** repair Song compatibility preflight separately and fold External Step's same-version guard defect into its existing revision.
	- **Rationale:** the Song loader defect predates the local stack, while the ineffective External Step marker belongs to `nl/patch-midi-sequencer-01`. This preserves origin-based revision ownership.
- **Decision:** stop only tracked row MIDI notes during choke cleanup.
	- **Rationale:** raw arp reset can strand notes, while channel-wide All Notes Off can silence unrelated rows or tracks. Existing tracked post-arp note ownership gives the narrow correct cleanup.

## Workers

- `/root`: completed plan, design integration, JJ topology, final validation, and operator communication; status: active only for physical follow-up.
- `/root/preview_trace`: completed runtime and file-by-file audits, integrated P48 implementation, focused tests, configured host suite, formatting, and affected ARM object compilation; status: completed.
- `/root/preview_review`: completed prerequisite schema work and both P48 persistence, compatibility, menu, localization, contract, and test reviews; final verdict: clean at P1/P2.
- `/root/owner_lookup`: completed owner/QMD lookup and both P48 runtime reviews; final verdict: clean at P1/P2.

## Implementation Steps

- [x] Lock the initial user-facing contract and excluded work.
- [x] Trace current Kit-wide CHOKE release and every note-on caller.
- [x] Trace row serialization, compatibility markers, cloning, presets, Slicer, menus, and localization.
- [x] Reconcile findings and update this plan before source edits.
- [x] Create and validate `ns/bug-song-load-01` before the saved-schema features.
- [x] Fold local schema 1 into `nl/patch-midi-sequencer-01` and rebase later local revisions once.
- [x] Create `nl/patch-kit-rows-02`, update repository guidance and self-contained deviation contracts, and implement schema 2 plus choke groups.
- [x] Add focused tests and run the review/repair loop.
- [x] Run final formatting, ARM compilation, host suite, and one Release build.
- [ ] Leave flashing and physical verification to the operator.

## Work Log

- [x] [[2026-08-30]] - Inserted and validated the non-destructive Song compatibility preflight.
	- **Evidence:** `ns/bug-song-load-01` is Jujutsu change `wxsvvosv` at commit `e48accd0`, between `nl/patch-kb-drums-01` and `nl/patch-midi-sequencer-01`. Seven focused tests, all 16 configured host tests, formatting, and five affected ARM Release objects passed. Independent review found no P1 or P2 issue.
	- **Effect:** Unsupported XML or JSON Song compatibility markers are refused before playback, UI, undo, or current-Song mutation. P45 can now own schema 1 and P48 can extend it to schema 2.
	- **Next:** Complete the P45 schema-1 fold, then create the P48 tip.
- [x] [[2026-08-30]] - Held P48 creation after compatibility review found three prerequisite edge cases.
	- **Evidence:** Reordered root fields can evade the current preflight; an empty firmware string can reach an unsafe parser read; and malformed local sentinel spellings can follow the numeric ignore path.
	- **Effect:** Repairs remain in `ns/bug-song-load-01` and `nl/patch-midi-sequencer-01`. No cancellation patch or P48 source exists yet.
	- **Next:** Complete origin-correct repairs and independent review before creating the choke-group revision.
- [x] [[2026-08-30]] - Closed prerequisite review and created the P48 tip.
	- **Evidence:** All accepted parser and compatibility findings were repaired in their origin revisions and final independent review found no P1 or P2 issue. Jujutsu created change `lrnvyroynrow` at commit `7820eebd` with bookmark `nl/patch-kit-rows-02` directly above `nl/patch-oled-03`.
	- **Effect:** P48 now has an empty, correctly based revision. No choke-group source or schema-2 behavior exists yet.
	- **Next:** Add deviations 26 and 27, then implement the locked design.
- [x] [[2026-08-31]] - Completed the first integrated P48 implementation and validation pass.
	- **Evidence:** `DEVIATIONS.md` now declares 27 deviations. The feature revision contains row state and persistence, Kit-owned grouped release, schema 2, contextual menu/localization, and focused tests. Four focused targets and all 23 configured host targets passed; formatting passed; 13 affected ARM Release objects compiled.
	- **Effect:** `nl/patch-kit-rows-02` is change `lrnvyroynrow` at commit `8819614a`. No full Release link, flash, Git operation, or physical claim occurred.
	- **Next:** Run split independent review, repair accepted findings, then perform the single final Release build.
- [x] [[2026-08-31]] - Accepted the first P48 review findings.
	- **Evidence:** Group release currently runs before Kit-arpeggiator resolution, so a duplicate or deferred arp input can silence its group without starting a replacement. Existing focused tests cover only pure policy helpers and do not execute production runtime, persistence roots, aggregation, or contextual menu routing.
	- **Effect:** P48 is not ready for a Release build. The release call must move to actual immediate start sites and the validation seam must cover production state and routing.
	- **Next:** Repair the runtime boundary, add production-facing tests, rerun focused/host/ARM checks, and repeat independent review.
- [x] [[2026-08-31]] - Closed P48 review and final software validation.
	- **Evidence:** The release scan now shares the resolved immediate-start gate with the replacement note. Eight production-linked cases exercise gating, state normalization and serialization, schema accumulation and markers, real JSON acceptance/refusal, contextual routing, generated display names, and all 16 group values. Four focused targets, all 23 configured host targets, formatting, and 13 affected ARM objects passed. Both final independent reviews found no P1 or P2 issue. The uninterrupted Release build passed 492 of 492 steps in 432.26 seconds. The owner doctor checked 4 groups and 48 plans with 0 errors and 0 warnings.
	- **Effect:** `nl/patch-kit-rows-02` is software-complete at `a09b3012`; no firmware was flashed and no Git operation occurred.
	- **Next:** Physical Deluge and real save/reload checks remain.

- [x] [[2026-08-30]] - Started P48 from the proposed choke-group contract.
	- **Evidence:** the current owner thread matches this task; P48 and plan-44 were unused; the local checkout and `kit-rows` revision group already exist.
	- **Effect:** the feature has one owner and one intended revision, with runtime, persistence, compatibility, and UI treated as one integrated change.
	- **Next:** complete the three read-only audits and revise any assumption that current source disproves.
- [x] [[2026-08-30]] - Completed owner, runtime, persistence, UI, and compatibility audits.
	- **Evidence:** QMD found no competing P48. Source tracing found one ordinary Kit start boundary, virtual fallback serialization for `SoundDrum`, root compatibility markers for Song/Kit/row presets, a direct CHOKE Select-navigation seam, and stable Drum identity through clipboard and reorder. It also proved that generic Sound Drum note-on is called inside Kit arpeggiator callbacks and that same-version `1.3.1` firmware cannot refuse the new data.
	- **Effect:** group scanning moves to Kit ordinary-start ownership; direct Waveform Editor preview remains protected; deletion wording is corrected; compatibility and row-arp cleanup remain explicit blockers rather than assumptions.
	- **Next:** resolve those two blockers, then freeze the implementation contract.
- [x] [[2026-08-30]] - Resolved compatibility and row-arpeggiator cleanup design.
	- **Evidence:** numeric and prerelease markers can collide with future upstream versions; exact `nl-save-schema-N` sentinels fail closed in unpatched firmware. Song load currently discovers incompatibility only after destructive state changes and discards the error. The existing tracked post-arp note arrays can emit exact row-owned note-offs without the channel-wide All Notes Off message.
	- **Effect:** P48 now owns a stable schema-2 contract, a separate pre-existing Song-load dependency, an origin-correct schema-1 fold for External Step, and a balanced audio/MIDI row release sequence.
	- **Next:** prepare the three revisions without leaving a cancellation patch.

## Validation

- **Command or action:** source, owner, and compatibility audits.
	- **Expected result:** identify the established choke seam and exact persistence/menu patterns before source changes.
	- **Observed result:** passed. Both initial blockers now have exact designs and validation matrices.
	- **Failure signals:** a render-time-only choke seam, incompatible row ownership, no guarded save mechanism, or a menu structure that makes nested CHOKE configuration ambiguous.
	- **Interpretation:** implementation can begin through one pre-existing bug revision, one fold into the prior saved-data feature, and one P48 feature revision.
- **Command or action:** production-linked focused checks, configured host suite, formatting, affected ARM Release objects, split independent reviews, and one logged Release build.
	- **Expected result:** resolved starts release before replacement; deferred Kit-arp input does nothing; saved group state and schema markers remain exact; displays route correctly; all software gates pass without P1/P2 findings.
	- **Observed result:** passed. Eight P48 cases, four focused targets, 23 of 23 configured host targets, formatting, 13 affected ARM objects, two independent reviews, and 492 of 492 Release steps passed.
	- **Failure signals:** choke without a replacement start, CC123, cross-group release, lost persistence, missing schema 2, wrong menu routing, failed test/object/build, or a P1/P2 finding.
	- **Interpretation:** software work is complete. Only device behavior and physical save/reload remain unverified.

## Unfinished Work

- [ ] Physically validate trigger timing, audition, arpeggiators, exact external MIDI note-off balance, OLED/7SEG navigation, effects tails, and Song/Kit/row save-reload.
