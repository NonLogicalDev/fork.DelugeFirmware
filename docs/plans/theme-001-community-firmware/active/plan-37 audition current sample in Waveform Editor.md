---
description: "Add a dedicated way to audition the exact Kit row being edited in Waveform Editor."
tags:
  - note-type/project
  - status/growing
  - domain/learning
  - deluge
  - firmware
---

# Audition current sample in Waveform Editor

## Purpose

Let the player hear the exact Kit row whose bounds they are editing, without leaving Waveform Editor or finding its row-position Audition pad again.

## Behavior Contract

- In a Kit row's Waveform Editor, holding Select auditions the exact Sound Drum being edited; releasing Select sends its matching note-off.
- Current Start, End, Loop, reverse, transpose, and Cut, Once, or Loop mode remain authoritative.
- The control follows the edited Sound Drum directly, not the visible waveform viewport, the current Kit selection, or a row inferred from vertical scroll position.
- Existing row Audition pads, marker editing, scroll, zoom, shortcuts, recording, MIDI, and saved data remain unchanged.
- Audio Clip and sampled Synth Waveform Editors keep their existing playback workflows. This exact-pad feature must not invent a Synth note or start song transport.

## Current State

- Status: software implementation, adversarial lifecycle review, targeted ARM objects, formatting, combined host suite, and combined Release build pass.
- Next action: verify the Select press-and-hold interaction on a physical Deluge; do not flash automatically.

## Revision

- Component group: `sample-editor`.
- Bookmark: `nl/patch-sample-editor-01`; Jujutsu change `vzymkworvqtn`; current commit `d019c57e2df5`.
- Keep the self-contained behavior contract in root `DEVIATIONS.md` at the newest feature revision.

## Workers

- `/root`: active owner; owns interaction choice, plan, integration, Jujutsu revision, final validation, and user communication.
- `/root/waveform_audition_input`: completed the read-only control audit; selected bare Select press-and-hold and identified release, Back, card-routine, and underlying Sound Editor ownership rules.
- `/root/waveform_audition_audio`: completed the read-only audio-path audit; selected direct Kit Sound Drum audition with pointer-based NoteRow lookup and explicit local ownership.

## Decisions

- **Decision:** Audition the edited Sound, not the waveform viewport.
	- **Rationale:** The user enters from a specific pad and needs to hear that pad after changing its bounds. Scroll and zoom are navigation state, not playback bounds.
- **Decision:** Prefer a momentary control with explicit note-on and note-off.
	- **Rationale:** It matches the Deluge's Audition interaction and avoids a hidden latched preview state.
- **Decision:** Use bare Select press-and-hold; keep Select rotation unchanged.
	- **Rationale:** Waveform Editor does not use Select press, global dispatch has no fallback action, and every alternative control already owns marker, zoom, transport, file, recording, modifier, or row behavior. Consuming both edges prevents the underlying Sound Editor from accepting a stray release if the editor closes while Select remains held.
- **Decision:** Do not reuse `UI_MODE_AUDITIONING` for the dedicated preview.
	- **Rationale:** That mode belongs to screen-position row Audition pads. The dedicated current-Sound preview needs independent ownership so simultaneous releases cannot stop or clear each other.
- **Decision:** Scope the exact-pad action to Kit Sound Drums.
	- **Rationale:** A Kit pad has one stable Sound Drum identity. A sampled Synth may map one Sound across ranges, notes, transpose, and an arpeggiator, so choosing a note would be new behavior rather than previewing an exact pad.
- **Decision:** Resolve the active Clip's NoteRow by the captured Sound Drum pointer and retain local preview ownership until release or editor exit.
	- **Rationale:** Vertical scroll and the selected Kit row may change while Select is held. Re-resolving either would risk stopping a different row or leaving the original voice active.
- **Decision:** Consume Select press and release, stop the local preview before Back or a normal row Audition action, and leave Select rotation available while held.
	- **Rationale:** This preserves marker editing and prevents two independent audition paths from competing for the Kit's single audition bookkeeping.
- **Decision:** Preview the Sound Drum directly and bypass only the Clip-level Kit arpeggiator.
	- **Rationale:** The Kit arpeggiator keys held input by Clip row number, which can change meaning after a Clip launch. The Sound Drum's own arpeggiator, choke, effects, sample bounds, playback mode, and row parameter state still apply.
- **Decision:** Let the Kit own one direct preview and end it before active-Clip changes or same-Drum sequenced, row, or MIDI notes.
	- **Rationale:** Ownership must remain valid across Panic, Back, row reorder, and external note paths. A later Select release becomes harmless after the Kit has already replaced or cancelled the preview.
- **Decision:** Do not layer the direct preview over an already-sounding voice or release tail on the same Sound Drum.
	- **Rationale:** Active-Clip cancellation must hard-stop Once and other no-tail preview voices. Starting only from a silent Drum ensures that reset cannot cut an unrelated same-Drum voice.

## Steps

- [x] Audit unused buttons and exact Sound audition APIs across Kit, sampled Synth, Keyboard/Pad entry, OLED, and 7SEG.
- [x] Record the accepted interaction and preservation rules before source edits.
- [x] Implement the smallest direct current-Sound audition path in a fresh Jujutsu change.
- [x] Add the self-contained deviation contract and proportionate source/object validation.
- [x] Run adversarial lifecycle review, fold or simplify every accepted finding, format, and compile the affected ARM objects.
- [x] Run the configured host suite and one logged Release build after the reopened P40 revision is updated, then record the final artifact evidence.
- [ ] Leave flashing and physical control-feel validation to the operator.

## Work Log

- [x] [[2026-08-25]]@00:09 - Completed the combined P40/P41 software gate and owner reconciliation.
	- **Evidence:** P41 is bookmarked as `nl/patch-sample-editor-01` at commit `d019c57e2df5` above P40 commit `1230870ec46b`. The configured CTest suite passed 11/11 in 1.31 seconds, and the single logged 48-target Release build linked in 56.36 seconds with BIN SHA-256 `484aafabd9c59952e176890525e4853af5dabf2daf687ee87a41a94ddddc5aae` and ELF SHA-256 `d63ce5405b4b1b3890785f29e13b2cd5c91b2d37e6658a717a57eddc10647ec3`. Tasker_Plunger doctor checked 4 groups and 41 plans with 0 errors and 0 warnings.
	- **Effect:** P41 is software-complete and remains active only for physical control confirmation. No firmware was flashed, published, or handled with Git.

## Validation

- Holding Select from a Kit row's Waveform Editor must sound that exact row after horizontal or vertical Kit scrolling; releasing it must stop the same row.
- Releasing it must deliver the matching note-off; Cut, Once, and Loop behavior must remain the Sound's existing behavior.
- Repeated presses, Back, marker editing while held, row Audition pads, card-routine deferral, and entry from Keyboard/Pad view must not leave a voice or UI mode latched.
- Sampled Synth and Audio Clip Waveform Editor playback behavior must remain unchanged.
- Formatting and the affected `sample_marker_editor.cpp`, `kit.cpp`, and `sound_drum.cpp` ARM Release objects pass. After P40 integration, the configured CTest suite passed 11/11 and the logged Release build linked in 56.36 seconds. The BIN SHA-256 is `484aafabd9c59952e176890525e4853af5dabf2daf687ee87a41a94ddddc5aae`; the ELF SHA-256 is `d63ce5405b4b1b3890785f29e13b2cd5c91b2d37e6658a717a57eddc10647ec3`. Physical Deluge confirmation remains required.

## Unfinished Work

- [x] Implement and locally validate the control.
- [ ] Confirm it on a physical Deluge.

Related: [[Synthstrom Deluge learning owner]].
