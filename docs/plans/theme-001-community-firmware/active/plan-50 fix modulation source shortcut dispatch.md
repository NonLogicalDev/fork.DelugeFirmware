---
description: "Keep a handled modulation source shortcut from also opening Randomizer."
tags:
  - note-type/project
  - deluge
status: active
---

# Modulation source shortcut dispatch

Ownership: [owner register](../../OWNERS.md). Related: [[plan-46 catalog firmware keybindings]].

## Scope

- Shift + LPF frequency, then Shift + Note or Velocity must leave the modulation amount open for that source and destination. A successful source shortcut must not also run the pad's ordinary Randomizer shortcut. Preserve Random source, second-layer modulation, two-source patching, permission checks, deferred input and Automation View updates.
- Preserve ordinary Randomizer shortcuts when modulation does not apply. No audio-engine, saved-data or gesture changes. Local fix, documentation, checks and Release build; no flashing or automatic publication.
- Confirmed in upstream `main` at `9e3132dc17070695b40f69bada5565a33ce428b4`: the initial ordinary candidate survives successful modulation navigation and is opened later. Use independent revision `ns/bug-modulation-01`, not an envelope fixup.

## Work

- Complete: /root applied the narrow dispatch fix, kept DEVIATIONS in the top feature revision, integrated checks and rebuilt through the established logged Git-free build harness.
- Complete: /root/modulation_review independently reviewed dispatch and upstream provenance, added a source-structure regression check and updated 19 affected catalog records. No blocking findings. Source-structure tests are not device or full dispatch integration tests. Existing unrelated source drift remains; the catalog is not fully current.
- Hardware acceptance: on a Synth and an individual Kit Sound Drum, open LPF frequency by shortcut and patch Note, Velocity and Random. Test fallback from a non-patchable screen, ordinary Randomizer shortcuts, horizontal and vertical menus, and Automation View. Hardware behavior is not yet verified.

## Verified result

- Software complete [[2026-09-14]]. Successful modulation navigation now returns after its existing Automation View callback, before the ordinary shortcut candidate can open Randomizer. Unhandled modulation still follows the existing fallback.
- Independent bug revision `ns/bug-modulation-01`: `d08ed27906ed13691a04c1730924fc6fed590f4c`. Integrated feature tip `nl/patch-envelopes-01` and local `nl/latest`: `b37869fdc41f89f6b79ef079cb3ca00ae86c8a4a`; empty clean child `1f2d2a`. DEVIATIONS section 31 remains in that newest feature revision. Recovery tag unchanged at `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`.
- `tests/source_contracts/test_modulation_shortcut.py` passes on the fix and fails on upstream's original handler. All 1,275 catalog records validate; all 12 catalog tests and 23 host test suites pass.
- One uninterrupted Release build ran 08:59:18–09:00:05 UTC, 47 seconds, exit 0. Log: `/private/tmp/deluge-modulation-shortcut-release.log`. Existing RWX LOAD-segment linker warning remains.
- Binary: `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware/build/Release/deluge.bin`, 1,792,308 bytes; SHA-256 `b5f19e676d53dab7f2d0096dc3bbf95c74bedf081875513b8bfa113dd3e022d0`; embedded version `v1.3.1-dev-b37869fdc41f`.
- No publication or flashing. Plan remains active for physical acceptance only; both workers finished their software work. No vault Git checkpoint because the operator prohibits direct Git.
