---
description: "Active plan for a local Jujutsu revision that removes Deluge KORG and Korg case collisions."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Consolidate duplicate Korg directories

## Purpose

Make the local checkout portable on a case-insensitive volume. This plan began on [[2026-08-21]].

## Scope

- In scope: create a local Jujutsu revision that moves files from `KORG` into `Korg` in the duplicate MIDI definition trees, preserve every file, and remove the sparse checkout.
- Out of scope: behavioral firmware changes, compilation, flashing, installation, or publishing the revision.

## Completion Criteria

- [x] Only `Korg` remains in the duplicate directory locations.
- [x] The merged tree contains the files from both prior spellings.
- [x] Jujutsu materializes the full checkout without case-only changes.

## Current State

- Status: completed.
- Result: local bookmark `local/fixups` at `5a5f59df80a783dd59b7fb98ecf7d79bad35461a` contains 20 exact `KORG` to `Korg` renames. It is the first local patch after `main`; the full checkout is materialized and clean.

## Work Log

- [[2026-08-21]]: Used a temporary Git index to verify that all duplicate-case files had distinct destination names and identical blobs, then imported the rename-only revision into Jujutsu. Nothing was pushed.
- [[2026-08-21]]: Reordered the local patch stack without changing patch contents. Renamed the Korg patch bookmark to `local/fixups`.
