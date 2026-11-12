---
description: "Active plan to stop the sample-browser preview when direct Manual slice opens the Manual Slicer."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Stop preview when opening Manual slice

## Purpose

Make the direct Manual slice entry quiet before the user starts placing slice boundaries. This plan began on [[2026-08-22]].

## Scope

- In scope: stop the current sample-browser preview when the direct Manual slice action opens Manual Slicer during kit creation.
- Keep stable: the existing Manual Slicer preview controls, standard Slice entry, Load all, sample selection, slice data, kit playback, saving, MIDI, and hardware installation behavior.
- Out of scope: stopping sequenced kit audio, changing the Manual Slicer algorithm or its in-editor preview, adding another menu action, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [x] Selecting Manual slice stops any active sample-browser preview before Manual Slicer opens.
- [x] Standard Slice and Load all retain their current preview and entry behavior.
- [x] No kit voice, project data, sample file, MIDI state, or stored setting is changed by the preview stop.
- [x] `DEVIATIONS.md` records the behavior as a self-contained, implementation-independent contract.
- [x] Focused tests where the source seam permits, plus host unit tests and a local Nix Release build, pass. No flashing or publication occurs.

## Current State

- Status: completed.
- Findings: the direct Manual slice action is the third choice in the kit sample-browser context menu. Its branch requests Manual mode and opens Slicer without stopping the sample-browser preview. The existing `AudioEngine::stopAnyPreviewing()` helper owns that preview and does not target kit voices.
- Decision: invoke the existing preview-only helper immediately before requesting Manual mode and opening Slicer. Do not stop a kit voice or broaden the behavior to standard Slice or other Slicer paths.
- Result: P14 is `nl/patch-sample-slicer-03` at `70b37a71544a8f95603214267c818f9c8a8cc78e`. It stops only the browser-preview owner before direct Manual slice opens. Host unit tests passed 148/148, the local Nix Release build passed, `git diff --check` passed, and independent source review found no issue.
- Worker: `/root` implemented and validated; `/root/manual_slice_entry_review` completed an independent scope and behavior review.
- Next action: a person with a physical Deluge should confirm the direct entry is quiet. Any Manual Slicer-specific audition issue remains separately owned by P15. Do not flash or publish.
