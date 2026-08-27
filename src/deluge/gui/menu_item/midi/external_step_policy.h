/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "io/midi/external_step_midi_input.h"
#include "model/clip/external_step_runtime.h"

namespace deluge::gui::menu_item::midi::external_step {

enum class InputRole : uint8_t {
	STEP,
	RESET,
};

enum class StatusLabel : uint8_t {
	SONG,
	OFF,
	UNASSIGNED,
	MISSING_INPUT,
	CONFLICT,
	UNSUPPORTED,
	WAITING,
	READY,
};

[[nodiscard]] constexpr bool shouldShowClockMenu(bool sessionMidiOut, bool featureEnabled, bool alreadyExternal) {
	return sessionMidiOut && (featureEnabled || alreadyExternal);
}

[[nodiscard]] constexpr bool shouldRememberHighStateAfterLearn(deluge::midi::ExternalStepMIDIMessageType messageType) {
	return messageType == deluge::midi::ExternalStepMIDIMessageType::NOTE;
}

[[nodiscard]] inline bool assignmentsAreIdentical(const deluge::midi::ExternalStepMIDIInput& first,
                                                  const deluge::midi::ExternalStepMIDIInput& second) {
	return first.hasSameAssignmentAs(second);
}

[[nodiscard]] constexpr StatusLabel statusLabel(deluge::external_step::Status status) {
	using enum deluge::external_step::Status;
	switch (status) {
	case SONG:
		return StatusLabel::SONG;
	case FEATURE_DISABLED:
		return StatusLabel::OFF;
	case UNASSIGNED:
		return StatusLabel::UNASSIGNED;
	case MISSING_INPUT:
		return StatusLabel::MISSING_INPUT;
	case CONFLICT:
		return StatusLabel::CONFLICT;
	case UNSUPPORTED:
		return StatusLabel::UNSUPPORTED;
	case WAIT:
		return StatusLabel::WAITING;
	case READY:
		return StatusLabel::READY;
	}
	return StatusLabel::UNSUPPORTED;
}

} // namespace deluge::gui::menu_item::midi::external_step
