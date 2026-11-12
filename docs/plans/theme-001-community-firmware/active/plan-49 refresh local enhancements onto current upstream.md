---
description: "Audit upstream, validate an isolated rebase, and apply the authorized live update with a recovery tag."
tags:
  - note-type/project
  - deluge
status: active
---

# Trial upstream refresh

## Result

Current live update ([[2026-09-13]]): authorized promotion and rebuild completed onto latest upstream `main`, `9e3132dc17070695b40f69bada5565a33ce428b4`. Active component tip is `08c87b08b14d938956dd89a85ad165b6d1bb2a87`, with clean empty working child `0f7af327e050d15ab5557aff4a7eb784d87dd1ee`. Old tip is protected by tag `local/pre-rebase-2026-09-13`. All 23 live host suites and Release pass. The earlier trial-only statements below are historical and superseded by this explicit live-update authorization.

Live Release ran uninterrupted from `21:36:56Z` to `21:38:59Z` (123 seconds), exit 0. Binary `build/Release/deluge.bin`: 1,792,124 bytes; SHA-256 `58259b2b293fc18680eba2e1acffc1dab94ccfc97aa3d66f3b4a34267f5dbb4f`. Owner verified log, hash, size, current revision and recovery tag. Only existing RWX linker warning remains. Build uses live-local CMake paths and Git-free metadata; no trial dependency paths remain. Catalog schema passes but the same 38 source files await review. Hardware checks and External Step LAST limitation remain explicit. No flashing or publication.

Rebuild requested [[2026-09-13]]: verified isolated tip `08c87b08b14d938956dd89a85ad165b6d1bb2a87` and no existing build; ran the existing Git-free harness once. Exit 0 at `2026-09-13T21:22:10Z`; compilation/link outputs were already current, so only version-generation checks ran. Binary remains 1,792,088 bytes, SHA-256 `64fa4ae2081b34099da542787eecfa1b58af3d9173fa99fa174caefe7c1ef62e`. Log: `/private/tmp/deluge-upstream-trial.VGJWkF-validation/recompile-latest-20260913.log`. No fetch, source edit or live-checkout change.

Trial complete; no live update applied. Candidate `08c87b08b14d938956dd89a85ad165b6d1bb2a87` preserves all 34 local revisions on upstream `9e3132dc17070695b40f69bada5565a33ce428b4`. Host tests and Release build pass. It is suitable for controlled hardware testing, not yet a claim of full behavioral parity: External Step LAST is explicitly inactive, 38 catalog source files need review, and hardware checks below remain. Applying this candidate to the live checkout requires separate authorization.

Authority: [OWNERS.md](../../OWNERS.md). Related: [[plan-48 add fine envelope timing controls]]. Requested through task `01a03bf2-27d5-7b80-8dbf-28b6027d1a3e`; that task is a recipient, not an additional owner.

## Outcome and boundaries

- New authorization [[2026-09-13]]: operator requests rebasing the live stack onto latest upstream and tagging the current version for recovery first. This supersedes the earlier trial-only restriction for this operation. Preserve an exact fixed pre-rebase reference and current binary, refresh upstream once, reuse reviewed trial resolutions where applicable, retain component bookmarks, validate and rebuild. No flash, push or publication.
- Recovery created before fetch/rebase: JJ tag `local/pre-rebase-2026-09-13` points to exact original tip `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`. Tag made that history immutable; JJ automatically created empty working child `d4fe1f`. Preserve the tag and its ancestry, do not override immutability to rewrite it. Old BIN copied to `_owner/artifacts/firmware/pre-rebase-2026-09-13-9e6a5e0f48f9.bin`, SHA-256 `26ce6b7d544d09d1d50fe879f211d08628445f195b53b7d1a299290eff2096cc`.
- Check current SynthstromAudible/DelugeFirmware upstream once, inventory the exact delta, and trial-rebase the full local stack in an isolated temporary copy.
- Keep `/Users/nonlogical/Projects/remote/github.com/SynthstromAudible/DelugeFirmware` unchanged. No fetch, snapshot, rebase, build or artifact replacement in that live checkout.
- Use JJ only; no Git commands. Do not push, flash, install or publish. Preserve all local behavior contracts, component bookmarks and ordering.
- Report upstream range, relevant changes, conflicts/resolutions, semantic overlaps, tests/build results, trial path and whether a live update is advisable. A successful trial does not authorize applying it live.

## Starting evidence

- Live local tip after envelope completion: `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`, `nl/patch-envelopes-01`.
- Last recorded upstream base: `685b4fb9d0219454b3ba87864640f3224bf16b56`; verify against actual ancestry before fetching.
- Full tree, local Korg casing fix first. DEVIATIONS.md contains 30 behavior contracts. Build environment is `.nix`; existing Nix access requires scoped escalation, never repeat the known sandbox probe.

## Execution

- Live tests and Release running in session 21325; log `/private/tmp/deluge-live-refresh.V3HqQE/validation-release.log`, harness `/private/tmp/deluge-live-refresh.V3HqQE/validate-build.sh`. Static version metadata identifies reviewed feature tip `08c87b08b14d`; all source/dependency/build paths are live-local, with no mutable trial dependency. No source edits during this process. Worker verified origin is the only remote and temporary tracking refs are gone.
- Live promotion verified by owner: component tip `08c87b08b14d938956dd89a85ad165b6d1bb2a87`; empty working child `0f7af327e050d15ab5557aff4a7eb784d87dd1ee`. Tree diff between them is empty. Recovery tag remains original `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`. Worker is verifying temporary remote cleanup after transient index-lock contention; no lock was manually deleted. Live validation/build next.
- Live freshness check: one origin fetch confirms `main` is still `9e3132dc17070695b40f69bada5565a33ce428b4`; no `master` branch advertised. The exact reviewed candidate `08c87b08b14d938956dd89a85ad165b6d1bb2a87` was imported via temporary local JJ remote with 33 component refs and 34 ancestry changes, without auto-tracking. Worker is promoting exact-ID bookmarks and moving only the empty working child; immutable rollback history stays intact.
- Completed live promotion: /root/upstream_trial performed freshness check, safe import preserving tagged ancestry, all 33 component-bookmark updates and one logged live Release build. /root verified artifacts and recovery. Earlier completed trial entries below remain historical evidence.
- Completed: /root owns shared records and report integration.
- Completed: /root/upstream_trial created the isolated copy, fetched once, resolved/reviewed the trial rebase and ran validation.
- Completed: /root/upstream_overlap returned the semantic audit below; the trial worker received its findings.
- Pending operator decisions: whether to proceed with hardware testing and later authorize a live update; catalog refresh is separate unfinished documentation work.
- Preserve a running build; do not edit build inputs or start duplicate builds. Report real compile failure separately from a healthy slow link.

## Log

- [[2026-09-13]]: Envelope implementation finished before starting this independent work. Plan allocated with the current Tasker_Stream helper. No upstream repository work has started yet.
- Live read-only baseline: 34 descendant commits above main, including one existing unbookmarked commit `1c18f0ad13bf` that must not be silently removed. Local tip and full ordered bookmark list verified. SHA-256: DEVIATIONS.md `8d2fb8e04c2243f2d500eaa06ce602c85f9c03fccd7af62b8b02ab62169a8cbb`; AGENTS.local.md `c07460babd352304c1a613a49fb23a133b230bb2688bb3978459e5f5f5889145`; live BIN `26ce6b7d544d09d1d50fe879f211d08628445f195b53b7d1a299290eff2096cc`.
- Worker created isolated copy `/private/tmp/deluge-upstream-trial.VGJWkF`; verified relative JJ storage resolves to its own .git directory, with no object alternates or metadata symlinks. Fetch is confined to that copy. Fresh build configuration must avoid live absolute build/cache paths.
- Single fetch complete. Exact upstream range `685b4fb9d0219454b3ba87864640f3224bf16b56` → `9e3132dc17070695b40f69bada5565a33ce428b4`, 50 commits. Owner read the actual commit list; three recent ParamManager changes are reverted at the tip and must not be counted as remaining improvements. Worker is trial-rebasing all 34 local descendants in the copy.
- Trial has conflicts inherited through 11 descendants; exact originating paths/resolutions pending worker report. Independent review identified upstream Shift-held Sound Editor shortcut overlay and per-device MIDI Thru as semantic integration checks.
- Owner verified encoder fix moves task blocking before event draining, without changing gesture handlers. Upstream optimization change adds global fast-math plus Release loop splitting/unswitching while retaining O2; do not describe it as a full O3 switch or claim measured performance improvement.
- Root conflict resolutions reported so far: combine independent ShortcutOverlay/sustain feature fields; retain upstream ClipView overlay plus local OLED ruler members; Audio Clip entry performs both overlay cleanup and local marker cleanup; Sample loop-point retains local explicit Select entry and upstream const relevance signature; local bounded JSON parser already fixes upstream's assignment typo and is retained with required includes.
- New upstream FIRST/LAST iterance needs explicit External Step treatment. Trial direction: FIRST uses the Clip's independent repeat count, N-of-M retains local semantics, and LAST must not use the shared Song clock. If no defined final external iteration exists, LAST remains unsupported/never fires and is reported as a live-update decision, not silently treated as parity. Semantic review pending.
- Follow-up review confirms External Step has no scheduled final iteration: pulses wrap the local repeat count, reset/stop return to pre-zero, and timeout cuts notes without advancing phase. FIRST is local repeat zero; LAST remains false in the isolated adaptation. Worker will record this in the trial DEVIATIONS contract. Final review waits for a stable tip rather than reading intermediate revisions as final source.
- Trial worker reports conflict-free 34-revision candidate e77bab; const API adaptations included for local menus. Copied absolute toolchain/current link is being replaced with a copy-local link before validation.
- No-Git validation constraint: upstream DBT itself calls Git, CMake calls it for version metadata, and fresh host FetchContent may clone Git dependencies. Authorized trial-only direct CMake under the same .nix toolchain, independent copied dependency sources with offline settings, and a narrowly scoped metadata override using verified JJ identity. It must cover generated version-script subprocesses too, preserve all nonmetadata command behavior and be recorded as a build harness rather than an unmodified DBT build. No live or audio-source mutation authorized.
- Stable-candidate review found one P2 semantic integration defect: new Shift shortcut pads can remain drawn beneath ScaleMenu because global Scale interception bypasses native dismissal. Authorized narrow dismissal and native root redraw on Scale-menu entry, folded into the trial scale-controls revision; no new input mode. Build waits for this fix and re-review. Reviewer confirmed External Step ending handling and bounded JSON/preflight logic otherwise retained.
- Scale fix in owning trial revision `4767f2`: reuse stopShortcutOverview on Instrument/Automation roots on entry, with no input-mode changes. Candidate `08c87b08b14d938956dd89a85ad165b6d1bb2a87` submitted for final review and validation.
- Actual harness: static metadata adapter supplied as GIT_EXECUTABLE and a PATH alias for generated metadata subprocesses. It does not invoke Git; supports only version/short-revision/status queries and fails all other requests. No build-system source edits. Independent test dependency copies and fresh build directories are used. Host checks and logged Release starting; exact log/session pending.
- Final targeted independent review at `08c87b08b14d938956dd89a85ad165b6d1bb2a87` found no remaining introduced P1/P2 defects in reviewed conflict resolutions. Scale cleanup preserves native routing; External Step explicitly lacks LAST; JSON/preflight safety retained. This is not full-stack or hardware parity proof. Catalog source drift from upstream must be reported, not blindly recertified.

## Semantic audit and hardware checks

- Validation progress: all 23 host suites passed in approximately 30 seconds. Initial firmware configure stopped before compilation when Argon FetchContent requested a clone; the metadata adapter refused it, with no Git executed. Worker is providing independent Argon/ETL source copies and explicit disconnected overrides before the actual Release compile. No running build was interrupted.
- Release compilation started `2026-09-13T20:26:36Z`, shell session 4233, log `/private/tmp/deluge-upstream-trial.VGJWkF-validation/release.log`; last worker observation 52/510 targets. Preserve this process across turns and do not edit inputs. Host and initial configure evidence: `/private/tmp/deluge-upstream-trial.VGJWkF-validation/validation.log`.
- Owner inspected `/private/tmp/deluge-upstream-trial.VGJWkF-validation/jj-build-metadata.sh`: three static responses only; no Git invocation, all other requests fail. Adapter SHA-256 `938db7549027c3b98cec491c85224ffe26c261872280d108fa63f759252aec92`. `build.sh` configures Ninja Multi-Config with disconnected copied Argon/ETL sources and builds Release with parallel 8. Embedded revision is explicitly `08c87b08b14d-trial`.
- Release passed exit 0 at `2026-09-13T20:28:33Z`, 117 seconds after start. Owner independently checked final log and artifact: `/private/tmp/deluge-upstream-trial.VGJWkF/build/Release/deluge.bin`, 1,792,088 bytes, SHA-256 `64fa4ae2081b34099da542787eecfa1b58af3d9173fa99fa174caefe7c1ef62e`. Only the existing RWX LOAD warning remains. Catalog audit result pending; source hashes must not be silently refreshed.
- Catalog result: schema validates 1,275 bindings and 12 tests pass. Source audit exits 1 because 38 referenced files changed; zero unaccounted handler candidates or invalid coverage rows. Existing 193 potential gesture groups are not proven conflicts. Drift was preserved, not recertified; details `/private/tmp/deluge-upstream-trial.VGJWkF-validation/catalog-audit.log`.
- Worker final live-file hashes match all owner baseline hashes for DEVIATIONS.md, AGENTS.local.md and built BIN; live tip remains `9e6a5e0f48f9eccbbbb57e9b4f9668267dec0668`. No live update, flash or publication.

- FIRST/LAST iterance: upstream `3ce3d41df2a5` adds an ending argument to note conditions. External Step must retain its independent repeat count and not borrow Song phase. LAST has no predictable external final pulse.
- Sound Editor `ff4d7f7eeef1`: new default-on Shift shortcut overlay, shortcut resolution and const virtual signatures affect local overrides. Check Scale menu with overlay on/off, release order, audition/Vertical, and sample-info → Select → waveform entry.
- Note Editor `4578793c186a`: explicit audition-muted state replaces the velocity sentinel. Preserve it alongside per-note Horizontal preview ownership; test both release orders and ordinary recording.
- Envelopes: upstream initializes smoothed sustain through noteOn; it does not change ADR time mapping or lower minimum Decay. Retain new initialization and verify fine control/base-time display.
- Audio: preserve new shared-output conversion guards alongside repeated recording repair. Names now persist and voice counts follow render state. Test replacement recording, monitoring, stop/start and conversion refusal with shared outputs.
- DX7 `8481f0be9f59`: object pool and source cleanup do not replace local CC64 sustain. Check patch replacement, held sustain, exhausted/reused voices and Panic. Browser Back fixes remain independent.
- MIDI `83013cb4eb83`: per-device Thru must not receive consumed External Step messages; unrelated MIDI forwarding must remain intact with sustain. Check Thru on/off per device.
- Serialization: retain JSON state and writer reset/buffer fixes plus local non-destructive compatibility preflight and saved schemas. Check XML/JSON incompatible rejection before mutation and compatible round trips.
- Performance flags require audible/numerical parity checks, particularly DX7, reverb and high-polyphony monitoring; no measured performance improvement claimed.
- No aggregate upstream slicing or waveform-navigation algorithm changes found. Neighboring browser/shortcut changes still require second-batch slicing, confirmation, audition and sample-bound navigation checks.
- USB MIDI pipelined-send changes were also reverted; do not count them or the three reverted ParamManager changes as current improvements.

## Exact conflict resolutions

All paths below are under `src/deluge/` in the isolated copy. Ten distinct paths conflicted; resolutions remain folded into their owning component revisions.

| Owner revision | Paths | Resolution |
| --- | --- | --- |
| nl/patch-midi-input-01 | gui/menu_item/runtime_feature/settings.cpp; model/settings/runtime_feature_settings.cpp; model/settings/runtime_feature_settings.h | Retain upstream ShortcutOverlay and local sustain fields/options. |
| nl/patch-oled-02 | gui/views/clip_view.h | Retain upstream overlay state and local ruler state. |
| nl/patch-audio-clip-01 | gui/views/audio_clip_view.cpp | Run both upstream overlay dismissal and local marker cleanup. |
| nl/patch-sample-editor-02 | gui/menu_item/sample/loop_point.h | Keep explicit Select entry and adopt const relevance signature. |
| ns/bug-song-load-01 | storage/JsonDeserializer.cpp | Preserve bounded EOF/escaped-string handling and local preflight with necessary upstream includes; assignment typo was already fixed locally. |
| nl/patch-midi-sequencer-01 | gui/ui/menus.cpp; storage/Deserializer.cpp; model/clip/instrument_clip.cpp | Keep per-device Thru and namespace alias, cluster/compatibility includes, and explicit ending state in shared note resolution. |

Additional source adaptations: External Step menu const relevance overrides, choke Polyphony const relevance override, pending-note helper declaration in instrument_clip.h, and Scale-menu entry overlay dismissal in scale_menu.cpp/clip_view.h. External Step FIRST follows local repeat count and LAST remains false; Song-clocked clips keep upstream ending behavior. Trial DEVIATIONS.md retains all 30 contracts and records the iterance distinction.

## Artifacts and reproduction evidence

- Trial repository: `/private/tmp/deluge-upstream-trial.VGJWkF`.
- Validation directory: `/private/tmp/deluge-upstream-trial.VGJWkF-validation`.
- `stack.txt` contains all 34 exact revision/bookmark identities, including the existing empty unbookmarked revision. `local/fixups` remains first.
- `build.sh` contains the direct CMake Release command; SHA-256 `cd6474b8b4fd0ac014cb5ddc884534b6a3eb83a07e96d2d884fe5378d747ea35`.
- `validate.sh` contains host-test commands; SHA-256 `4decac143c389c7d586844db9d1f8912c2c953eafbc2f4fda9ea4a1ff9a3b08b`.
- `release.log`, `validation.log` and `catalog-audit.log` preserve successful build, host tests/initial configure refusal, and honest source drift respectively. No Git executable was invoked in the isolated trial workflow; build metadata uses the documented static adapter.
- Final owner structure check: five groups, 54 plans, zero errors/warnings. Temporary artifacts are not an off-machine backup; do not delete the trial before the operator has reviewed it.
