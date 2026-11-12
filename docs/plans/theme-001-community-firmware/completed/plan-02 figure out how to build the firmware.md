---
description: "Active plan to provide a local Nix build environment for Deluge community firmware."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Figure out how to build the firmware

## Purpose

Make the upstream build path reproducible locally, including ordinary local build artifacts, without installing, flashing, or publishing firmware. This plan began on [[2026-08-21]] and is reopened on [[2026-08-22]] to add local compilation caching.

## Product integration

- Existing model: `.nix/` pins the official hard-float toolchain and exposes the ordinary DBT/CMake build path without requiring a global compiler installation.
- User intent: repeat local firmware builds should avoid recompiling unchanged source while preserving the exact compiler and output behavior.
- Integrated model: the Nix development environment provides `ccache`, gives this checkout a persistent ignored cache directory, and configures C and C++ compilation through that cache. A normal DBT build remains the only build command.
- Architecture impact: build-environment only. The cache sits between CMake and the existing cross compiler; it does not replace, wrap, or change the locked compiler, linker, build driver, firmware source, or artifact location.
- Existing behavior preserved: a cold cache still completes the same local build, cache misses use the official v22 cross compiler, and no environment action flashes, installs, publishes, or uploads firmware.

## Scope

- In scope: inspect upstream build entry points, create `.nix/` with the required development dependencies, add `ccache` to that environment, and validate that the environment evaluates and exposes its tools.
- Out of scope: firmware behavior changes, a different compiler or linker, cache sharing or uploading, firmware installation, SD-card changes, or hardware testing.

## Completion Criteria

- [x] `.nix/` states the build dependencies and usable entry command.
- [x] The Nix environment evaluates successfully.
- [x] Its tool set is checked against the upstream build instructions.
- [x] The environment provides `ccache` and directs C/C++ compilation through it without changing the selected v22 toolchain.
- [x] A cache directory under the ignored repository `.cache/` is used consistently for this checkout.
- [x] Reconfiguring and building locally still produces the ordinary Release artifacts, and the configured CMake launchers show `ccache`.

## Current State

- Status: completed. The resumed audit proved that build caching was already implemented and folded into the existing Nix-environment revision; only the owner records were stale.
- Result: `.nix/` provides a locked aarch64-Darwin environment. Start it from the repository root with `nix develop path:./.nix`.
- Validation: it pins the upstream v22 hard-float toolchain, rather than Nix's incompatible soft-float runtime. The normal `DBT_NO_SYNC=1 ./dbt build Release` command completed successfully on [[2026-08-21]] in P8.
- Decision: use `ccache` only as CMake's compiler launcher. Keep cache files in the ignored checkout-local `.cache/ccache` directory, so no cache state is committed or shared implicitly.
- Result: `nl/patch-nix-env-01` remains Jujutsu change `zyyvmolzkvlmyrpokulwrupktooklvzo`, commit `fdce0f9150e3f91c0781304fad481a1333a88016`. No repository edit or revision rewrite was needed during the final reconciliation.

## Workers

- `/root`: completed P6 plan reconciliation and final validation; no remaining software or repository work.
- `/root/ccache_nix_audit_2`: completed the read-only `.nix/`, DBT/CMake, ignore, deviation, generated-state, and Jujutsu ownership audit; correctly found no remaining project edit.

## Work Log

- [[2026-08-21]]: Replaced the initial generic Nix cross-compiler approximation after its ABI-incompatible runtime failed at link time. The flake now pins the upstream toolchain tarball by SHA-256 and keeps the build local.
- [[2026-08-22]]: Reopened P6 after a long local rebuild exposed repeat-compile cost. Add and verify `ccache` in this existing environment revision, then fold the completed change back into `nl/patch-nix-env-01`.
- [[2026-08-24]]@20:36: Resumed the previously delegated P6 work after verifying the owner thread and runtime guidance. The checkout guidance already describes `ccache`, while this plan still records the implementation as unfinished, so the first action is a read-only reconciliation before changing source or revision state.
- [[2026-08-24]]@20:37: Paused P6 immediately when the operator corrected P40's OLED ruler model. The read-only audit worker was interrupted; no repository file, revision, build, or validation state changed for ccache.
- [[2026-08-24]]@21:40: Resumed P6 after the full-width waveform build and owner gate completed. The first action is a fresh read-only reconciliation because checkout guidance claims `ccache` is already configured while the plan still records every cache criterion as unfinished. No source or revision change is allowed until the current state and owning Jujutsu revision are proven.
- [[2026-08-24]]@21:44: Completed P6 after current-state reconciliation and a focused Nix-shell validation. `.nix/flake.nix` supplies `ccache`, exports the checkout-local cache and both compiler launchers, `.gitignore` excludes `.cache`, and the self-contained deviation contract already describes the workflow. The configured CMake cache selects the locked v22 ARM C/C++ compilers and records `ccache` for both launchers; real Release Ninja edges use that launcher. The Nix shell exposes `ccache` 4.13.6 at `.cache/ccache`, and its accumulated statistics show 8,105 cacheable calls, 2,953 hits, 5,152 misses, a 36.43% hit rate, and 1.9 GiB of local cache data. The successful 47.33-second Release build at 21:35 updated this cache and produced the ordinary ELF and BIN. `nl/patch-nix-env-01` is non-empty, conflict-free, and already owns the three `.nix/` files; no repository or Jujutsu change was warranted.

## Validation

- [x] `ccache` 4.13.6 is executable inside `nix develop path:./.nix`.
- [x] The existing configured build records the pinned v22 C and C++ compilers plus `ccache` as both compiler launchers; real Release Ninja compilation edges use it.
- [x] The normal Nix Release build completed and produced `build/Release/deluge.elf` and `build/Release/deluge.bin` without flashing or publishing.
- [x] Cache statistics after compilation show active misses and 2,953 hits, proving that the launcher and persistent checkout-local cache are in use.

## Unfinished Work

None. Recheck only if the Nix lock, official toolchain version, CMake configuration model, or checkout cache policy changes.
