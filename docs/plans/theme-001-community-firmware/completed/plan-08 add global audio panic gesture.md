---
description: "Completed local Panic action for immediate audio silence from the Sound Editor settings menu."
tags:
  - note-type/project
  - status/mature
  - domain/learning
  - deluge
  - firmware
---

# Add Panic as first Sound Editor menu action

## Purpose

Add a visible Panic action that silences active audio immediately without damaging a song or its saved state. This plan began on [[2026-08-22]].

## Scope

- In scope: add Panic as the first selectable action in the existing Shift + Select Sound Editor settings menu during normal operation; the Panic routine itself stops active sample preview, voices, scheduled audio events, runtime effect tails, live recording, and stem export through established safe paths.
- Keep stable: opening and navigating the existing menu, normal knob actions, saved project data, undo history, storage, MIDI configuration, and hardware installation behavior.
- Out of scope: clearing song data, deleting samples, clearing an audio buffer without verified need, resetting device settings, flashing, installation, publishing, or upstream submission.

## Questions to resolve

- Which Sound Editor root menu owns the first selectable action, and how does an immediate action return control after it runs?
- Which existing stop paths cover sample previews, voices, and queued audio events, and what does each leave running?
- Is an audio-buffer clear necessary after those stop paths, or would it create an avoidable discontinuity or race?

## Acceptance Criteria

- [x] The existing Shift + Select Sound Editor settings menu presents Panic as its first selectable action, without changing how its other actions are reached.
- [x] Selecting Panic in normal operation invokes one global silence action that stops active playback, audible voices, sample previews, audition notes, MIDI and gate notes, queued audio events, and delay, modulation, compressor, filter, and reverb tails through safe existing behavior.
- [x] The Panic routine aborts active audio recording and stem export if a future caller invokes it in those modes, while preserving saved project data, settings, automation, and undo history. The Settings-menu entry need not be reachable in those modes.
- [x] No song, preset, sample, setting, undo, or MIDI data is changed; buffer clearing is excluded unless source evidence proves it necessary and safe.
- [x] The action gives clear local feedback and leaves the interface in a usable state after it runs.
- [x] `DEVIATIONS.md` records the behavior as a self-contained, implementation-independent contract.
- [x] The existing host unit suite passed with 148 tests and 307,361 checks, and two local Nix Release builds linked `build/Release/deluge.elf`. A focused host test was not added because the complete action crosses the audio engine, recorder/card lifecycle, playback handler, and hardware-oriented model state. No flashing or publication occurred.

## Current State

- Status: completed locally; physical-device validation remains before any flashing decision.
- Findings: an immediate Shift + Select + Tempo chord cannot prevent the earlier Select action from opening this menu or the earlier Shift + Tempo action from clearing tempo automation. Shift + Select reaches `settingsRootMenu`, whose first child is highlighted on entry, so a direct first Panic leaf needs no input-dispatch change. Existing safe stop paths cover playback, MIDI and gate note-offs, audition notes, and sample-browser preview. `killAllVoices()` alone is not sufficient: it leaves delay, modulation, compressor, filter, and reverb tails. The low-level transmit buffer is intentionally not fully zeroed by normal rendering and has no established safe clear call site. The existing Settings menu is not reachable while direct recording or stem export owns the controls. That limits only current access: the reusable Panic routine aborts those modes when a later caller can invoke it.
- Decision: make Panic the first explicit action in the global Settings menu for normal operation. It must end playback, stop note and preview owners, silence active sound and audio-clip voices, clear musical effect state so feedback tails end, and abort live audio recording and stem export whenever any caller invokes it. It remains selected after feedback, rather than exiting through a path that would save settings. It must not clear a low-level audio buffer, alter saved song data, or run a destructive partial knob shortcut. Recording and stem export retain their established controls until a later task provides a suitable entry point.
- Revision: Panic is a distinct core capability at `nl/patch-panic-01` (`22af003c0604`).
- Implementation: `/root/panic_implementation` added the first Settings action and localized stopped feedback. `AudioEngine::panic()` stops playback, MIDI/gate notes, auditions, previews, voices, audio clips, delay, stutter, modulation, granular state, filter, compressor, reverb, and input monitoring. It safely aborts active recorder and stem-export work for a later caller, and does not clear the raw output buffer. The reverb reset reconstructs the active model while retaining its configured parameters.
- Verification: independent source review approved the completed corrections. The host suite passed with 148 tests and 307,361 checks. The local Nix Release build completed twice and linked `build/Release/deluge.elf`.
- Worker: `/root/panic_gesture_input`, `/root/panic_menu_design`, `/root/panic_audio_teardown`, `/root/panic_implementation`, and `/root/panic_review` completed their scoped work; `/root` completed integration and validation.
- Next action: a person with the physical Deluge can test the normal-mode Panic menu action before choosing whether to flash a local build. Do not flash or publish automatically.
