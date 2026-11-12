---
description: "Active correction plan to replace the surprising Manual Slicer stop gesture with established preview behavior."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Use coherent Manual Slicer preview stop behavior

## Purpose

Keep Manual Slicer audition stopping predictable, without a separate escape-shaped shortcut. This plan began on [[2026-08-22]].

## Scope

- In scope: remove the surprising Shift + Back stop gesture; retain the existing press-and-hold slice audition model and ensure plain Back stops the scoped audition as it leaves Manual Slicer.
- Keep stable: selected sample and slice data, subsequent pad auditions, the P14 browser-preview stop, standard Region slicing, kit playback, saving, MIDI, and hardware installation behavior.
- Out of scope: killing all voices on a kit drum, stopping sequenced audio, changing global preview behavior, changing the slicing algorithm, flashing, installation, publishing, or upstream submission.

## Acceptance Criteria

- [ ] Releasing a held slice pad remains the immediate, in-place way to stop its audition.
- [ ] Plain Back stops the current scoped audition as it leaves Manual Slicer, without claiming an in-place global stop.
- [ ] Shift + Back no longer provides a separate Manual Slicer audio-stop behavior.
- [ ] The stops affect neither sequenced kit voices nor unrelated browser preview, and leave all slice data unchanged.
- [ ] A later pad press can audition the selected slice normally.
- [ ] `DEVIATIONS.md` replaces the former stop contract with a self-contained, implementation-independent description of the verified behavior.
- [ ] Focused tests where the source seam permits, plus host unit tests and a local Nix Release build, pass. No flashing or publication occurs.

## Current State

- Status: active.
- Findings: releasing a held slice pad already sends the scoped audition note-off, so the natural model is hold to hear and release to stop. The current Shift + Back path sends that same scoped note-off but is surprising and redundant. Plain Back already leaves Manual Slicer, but its current first-drum force-kill is broader than the scoped audition path. The reported continuing sound may instead have been the pre-existing browser preview; P14 now stops that source before direct Manual Slice opens. A literal delay or reverb tail is separate and should not be cleared by this correction.
- Decision: remove Shift + Back. Use the established scoped audition stop when plain Back leaves Manual Slicer; do not kill an entire kit drum or clear effect buffers. This makes the user-visible model: hold a slice pad to hear it, release to stop, Back stops and leaves.
- Worker: `/root/slicer_stop_interaction` completed source interaction research; `/root` owns implementation, review, and validation.
- Revision: this replaces P11's Shift + Back behavior. Fold the final behavior and revised `DEVIATIONS.md` contract into `nl/patch-sample-slicer-02`; do not add a `sample-slicer-04` revision that only cancels P11.
- Next action: make this narrow replacement by rewriting P11 after P12 is isolated. Do not flash or publish.
