---
description: "Implement precise envelope time editing without changing existing sounds."
tags:
  - note-type/project
  - deluge
status: active
---

# Fine envelope timing controls

Ownership: [owner register](../../OWNERS.md). Related: [[plan-47 open Root and Mode menu with Shift Scale]].

## Outcome and scope

- Add coarse and fine editing for envelope Attack, Decay and Release, with useful time readouts where the existing engine permits an accurate conversion. Sustain remains unchanged.
- Preserve the fixed-point audio engine, saved values, automation, modulation and existing sounds. Merely viewing a value must not quantize it. Do not claim finer UI reduces the engine's minimum time without measuring it.
- Verify existing Delay Sync dotted/triplet support and its navigation; add no duplicate mechanism when upstream already satisfies the request.
- Keep the implementation in a new component revision, update the self-contained behavior contract in root DEVIATIONS.md, test and rebuild locally. No Git, flash or publication.

## Integration

- Existing model: envelope menu segments edit patched parameters through the common parameter write and MIDI/automation pathways; displayed 0–50 values are not the stored resolution.
- Intent: precise short transients without changing existing patches or adding a competing envelope engine.
- Reuse native menu editing and parameter writes. Limit new conversion/display behavior to time segments; preserve patching shortcuts and Sustain.
- Determine actual time conversion and minimum stage length before choosing units and steps. Avoid a broad Decimal or parameter framework rewrite.

## Work and checks

- Completed: /root repaired the graphical overview regression introduced by fine editing: time values use hundredths, while graph coordinates assumed 0–50. Graph input now retains fractional precision on the established scale; Sustain and audio behavior are unchanged. /root/envelope_review found no other affected value consumers and verified geometry and markers fit the screen. Folded into `nl/patch-envelopes-01` at `33ceddc0b8e703435afc6fa43e96554d687bb4a0`; recovery tag unchanged.
- Software complete: /root integrated the feature, DEVIATIONS.md, catalog and local JJ checkpoint. Hardware validation remains pending.
- Completed source: /root/envelope_controls implemented envelope source and five focused tests (10,993 checks passed).
- Completed: /root/envelope_catalog added entries and refreshed final reviewed source hashes; schema, audit and 12 catalog tests passed.
- Completed: /root/delay_sync_audit verified existing delay controls read-only. Sound → FX → Delay → Sync already includes straight, triplet and dotted values; no new delay feature is needed.
- Pending: operator checks menu readability on hardware, fine edits while auditioning, both menu layouts, modulation/automation and saved patch playback.
- Starting checkpoint: 87172eb75934f9804b0eb755b00232a4f0ae89d0, nl/patch-scale-controls-01; no envelope source changes yet.

## Log

- [[2026-09-13]] graph repair: regression tests cover all 5,001 time values and 51 legacy/Sustain values. All 23 host suites, 12 catalog tests and 1,275-entry schema validation pass. Only the reviewed time_value.h catalog hash refreshed; unrelated upstream catalog drift remains. Release completed uninterrupted in 50 seconds (22:05:19–22:06:09 UTC), log `/private/tmp/deluge-envelope-graph-release.log`. BIN is 1,792,372 bytes, SHA-256 `266d1cfd9226d09a345f7e4936800925864430f9bd7054df4b7734bd5d43b41f`, version `v1.3.1-dev-33ceddc0b8e7`. Existing RWX linker warning remains. Working child clean; DEVIATIONS.md updated. Hardware verification pending; no flash or Git.
- [[2026-09-13]]: User authorized implementation and reiterated that every feature belongs in DEVIATIONS.md. Current source confirms full-width stored parameter values and existing Delay Sync type support. Physical behavior remains unverified.
- Created fresh local change vozpws (initial empty commit 33d01b), parent 87172e. Source and test implementation is underway; no build started.
- Accepted narrow UI design: plain Select changes one legacy unit; held Select changes 0.01 unit. Preserve raw off-grid precision and show the 0.00–50.00 amount with nominal base ms/sec on OLED. Keep existing parameter writes and patching navigation, Sustain unchanged. DEVIATIONS.md section 30 records the desired contract while implementation is underway.
- Dispatch review found existing physical Shift acceleration before leaf menu edits. Preserve it instead of modifying shared dispatch: Shift alone keeps native coarse behavior; Select is the fine modifier. This supersedes the earlier proposed Shift fine alias. Implementation and catalog workers notified.
- Delay audit: handwritten menus.cpp defines Sound → FX → Delay → Sync; model/sync.cpp supplies straight/triplet/dotted choices and delay.cpp applies their rate multipliers. Existing optional AltGoldenKnobDelayParams cycles those types by pressing Delay Rate. No duplicate delay implementation planned.
- Source frozen for build: segment.h/.cpp, time_value.h, tests/unit/envelope_time_tests.cpp and test registration. Independent review caught OLED hint clipping and decimal truncation in horizontal slots; worker corrected both before build, final review pending.
- One combined host suite/configure/Release process started, shell session 33505, log `/private/tmp/deluge-envelope-release.log`. Preserve this process; do not edit build inputs while running.
- Nominal endpoint calculation from current tables: amount 0 gives immediate Attack, Decay approximately 11.6334 ms and Release 5.8167 ms. At 25: 46.4399 ms, 4877.38 ms, 2407.82 ms. At 50: 2972.15 ms, 63405.96 ms, 27173.98 ms. These are base phase times, not hardware measurements. User informed that this UI change does not lower the minimum Decay.
- Final independent source review found no remaining actionable findings after display corrections. Tests cover pure amount arithmetic and nominal phase time; menu routing and exact raw-to-rate conversion have source review, not executable integration tests.
- Initial build process exited 1, 20:10:19–20:10:26 UTC: all 23 host suites passed; target compilation caught two StringBuf arguments needing `.c_str()`. Owner fixed exactly those calls, formatted the four C++ files and started corrected Release session 73993 with `/private/tmp/deluge-envelope-release-fixed.log`. The failed process was finished before edits; no running build was interrupted.
- Catalog worker initially passed schema/audit/12 tests at 1,275 bindings, 392 discovered definitions, 519 coverage rows and 187 files; final reviewed hash refresh requested after compiler fix.
- Corrected Release succeeded 20:11:09–20:11:55 UTC (46 seconds), exit 0, log `/private/tmp/deluge-envelope-release-fixed.log`. Binary `build/Release/deluge.bin`: 1,744,760 bytes, SHA-256 `26ce6b7d544d09d1d50fe879f211d08628445f195b53b7d1a299290eff2096cc`. Existing RWX linker warning remains. All 23 host suites and final 12 catalog tests passed; five focused tests contain 10,993 checks. No hardware check, flash or publication.
- Local checkpoint `nl/patch-envelopes-01`: `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`, JJ change `vozpwssnpstpruwunxqvullvmxvoqyvy`, parent `87172eb75934f9804b0eb755b00232a4f0ae89d0`. Contains source, tests, section 30 of DEVIATIONS.md and catalog. Build embeds parent identity with dirty suffix; the hash above identifies the actual artifact. Local AGENTS.local.md guidance is ignored and not included in JJ recovery.
