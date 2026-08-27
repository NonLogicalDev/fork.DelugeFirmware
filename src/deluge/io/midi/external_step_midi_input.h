/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with The Synthstrom Audible Deluge Firmware.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

class MIDICable;

namespace deluge::midi {

enum class ExternalStepMIDIMessageType : uint8_t {
	NONE,
	NOTE,
	CC,
	PROGRAM_CHANGE,
};

/// Exact input identity and transient edge state for one External Step control.
///
/// Unlike LearnedMIDI, this binding never wildcards the cable and never converts a physical channel into an MPE
/// zone. Runtime edge and conflict state are intentionally reset when the saved assignment is copied.
class ExternalStepMIDIInput {
public:
	static constexpr uint8_t kUnassigned = 0xff;
	static constexpr uint8_t kChannelCount = 16;
	static constexpr uint8_t kNoteCount = 128;
	static constexpr uint8_t kCCCount = 120; // 120 through 127 are Channel Mode messages in the input parser.
	static constexpr uint8_t kProgramCount = 128;

	ExternalStepMIDIInput() = default;
	ExternalStepMIDIInput(const ExternalStepMIDIInput& other) { copyAssignmentFrom(other); }
	ExternalStepMIDIInput& operator=(const ExternalStepMIDIInput& other) {
		if (this != &other) {
			copyAssignmentFrom(other);
		}
		return *this;
	}

	void clear() {
		cable = nullptr;
		channel = kUnassigned;
		messageType = ExternalStepMIDIMessageType::NONE;
		number = kUnassigned;
		clearRuntimeState();
	}

	void clearRuntimeState() {
		high = false;
		conflict = false;
	}

	void clearHeldState() { high = false; }

	/// Apply derived conflict state. A transition in either direction rearms a held control so messages swallowed by
	/// the higher-priority owner cannot suppress the first edge after the binding becomes usable again.
	[[nodiscard]] bool updateConflictState(bool newConflict) {
		if (conflict == newConflict) {
			return false;
		}
		conflict = newConflict;
		clearHeldState();
		return true;
	}

	[[nodiscard]] bool isAssigned() const {
		return cable != nullptr && channel < kChannelCount && numberIsValid(messageType, number);
	}

	[[nodiscard]] bool matches(const MIDICable& incomingCable, uint8_t incomingChannel,
	                           ExternalStepMIDIMessageType incomingType, uint8_t incomingNumber) const {
		return isAssigned() && cable == &incomingCable && channel == incomingChannel && messageType == incomingType
		       && number == incomingNumber;
	}

	[[nodiscard]] bool hasSameAssignmentAs(const ExternalStepMIDIInput& other) const {
		return isAssigned() && other.isAssigned() && cable == other.cable && channel == other.channel
		       && messageType == other.messageType && number == other.number;
	}

	/// Observe the asserted state for a Note or CC control and report only its rising edge.
	[[nodiscard]] bool updateHighState(bool newHigh) {
		const bool risingEdge = newHigh && !high;
		high = newHigh;
		return risingEdge;
	}

	/// Observe one already-matched control message.
	///
	/// Note and CC callers supply their current asserted state. Program Change has no release state, so each message is
	/// an edge regardless of the argument.
	[[nodiscard]] bool observeMessage(bool asserted) {
		if (!isAssigned()) {
			return false;
		}
		if (messageType == ExternalStepMIDIMessageType::PROGRAM_CHANGE) {
			return true;
		}
		return updateHighState(asserted);
	}

	[[nodiscard]] static constexpr bool numberIsValid(ExternalStepMIDIMessageType type, uint8_t candidate) {
		switch (type) {
		case ExternalStepMIDIMessageType::NOTE:
			return candidate < kNoteCount;
		case ExternalStepMIDIMessageType::CC:
			return candidate < kCCCount;
		case ExternalStepMIDIMessageType::PROGRAM_CHANGE:
			return candidate < kProgramCount;
		case ExternalStepMIDIMessageType::NONE:
			return false;
		}
		return false;
	}

	MIDICable* cable = nullptr;
	uint8_t channel = kUnassigned;
	ExternalStepMIDIMessageType messageType = ExternalStepMIDIMessageType::NONE;
	uint8_t number = kUnassigned;

	// Derived runtime state. Neither field is serialized or retained by assignment / Clip duplication.
	bool high = false;
	bool conflict = false;

private:
	void copyAssignmentFrom(const ExternalStepMIDIInput& other) {
		cable = other.cable;
		channel = other.channel;
		messageType = other.messageType;
		number = other.number;
		clearRuntimeState();
	}
};

} // namespace deluge::midi
