---
description: "Active plan to keep a local Jujutsu checkout of the Deluge community firmware."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Check out community firmware with Jujutsu

## Purpose

Keep the community firmware source available locally for later study. This plan began on [[2026-08-21]].

## Scope

- In scope: install Jujutsu in the Nix profile, clone the official `SynthstromAudible/DelugeFirmware` repository under `~/Projects/remote/github.com/`, and verify the remote and working state.
- Out of scope: building, changing source, choosing firmware, copying files to an SD card, flashing, installing, or testing on the Deluge.

## Completion Criteria

- [x] Jujutsu is available from the Nix profile.
- [x] The official repository is cloned at `~/Projects/remote/github.com/SynthstromAudible/DelugeFirmware`.
- [x] The remote identity and empty Jujutsu working change are verified.

## Current State

- Status: completed.
- Result: Jujutsu 0.44.0 is in the Nix profile. The official `origin` remote is present and the clean `firmware source exploration` change sits on `main` at `685b4fb9d0219454b3ba87864640f3224bf16b56`.
- Caveat: upstream has both `KORG` and `Korg` paths under `contrib/`; the full repository is fetched, but the sparse working copy omits `contrib/` on this case-insensitive volume.

## Work Log

- [[2026-08-21]]: Installed Jujutsu from Nix and cloned the official repository. No build, source edit, firmware selection, SD-card change, or hardware installation was performed.
