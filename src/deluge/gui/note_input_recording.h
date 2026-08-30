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
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace deluge::gui::note_input {

enum class RecordingTransition : uint8_t {
	FOLLOW_UI,
	SUPPRESS,
	NOTE_ON,
	NOTE_OFF,
};

template <size_t SlotCount>
class RecordingLifecycle {
public:
	RecordingTransition noteOn(size_t slot, bool previewOnly) {
		active_.set(slot);
		previewOnly_.set(slot, previewOnly);
		recordedNoteOn_.reset(slot);
		return previewOnly ? RecordingTransition::SUPPRESS : RecordingTransition::NOTE_ON;
	}

	void noteOnRecordingResult(size_t slot, bool recorded) {
		if (active_.test(slot) && !previewOnly_.test(slot)) {
			recordedNoteOn_.set(slot, recorded);
		}
	}

	RecordingTransition noteOff(size_t slot) {
		RecordingTransition transition = RecordingTransition::SUPPRESS;
		if (active_.test(slot) && !previewOnly_.test(slot) && recordedNoteOn_.test(slot)) {
			transition = RecordingTransition::NOTE_OFF;
		}

		active_.reset(slot);
		previewOnly_.reset(slot);
		recordedNoteOn_.reset(slot);
		return transition;
	}

	void clear() {
		active_.reset();
		previewOnly_.reset();
		recordedNoteOn_.reset();
	}

	[[nodiscard]] bool isActive(size_t slot) const { return active_.test(slot); }
	[[nodiscard]] bool isPreviewOnly(size_t slot) const { return previewOnly_.test(slot); }
	[[nodiscard]] bool hasRecordedNoteOn(size_t slot) const { return recordedNoteOn_.test(slot); }

private:
	std::bitset<SlotCount> active_;
	std::bitset<SlotCount> previewOnly_;
	std::bitset<SlotCount> recordedNoteOn_;
};

} // namespace deluge::gui::note_input
