/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Firmware is free software: you can redistribute it and/or modify it under the
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace deluge::midi_support {

/// Fixed deferred note-off state for the complete MIDI pitch domain.
///
/// Each slot stores release velocity plus one, leaving zero as the unoccupied value. This keeps live MIDI note-off,
/// retrigger, pedal-release, and cleanup paths independent of the firmware allocator.
class SustainDeferredNotes {
public:
	static constexpr int32_t kNoteCount = 128;

	[[nodiscard]] bool defer(int32_t note, uint8_t releaseVelocity) {
		if (!isValidNote(note)) {
			return false;
		}
		encodedVelocities_[static_cast<std::size_t>(note)] = static_cast<uint8_t>(releaseVelocity + 1);
		return true;
	}

	void erase(int32_t note) {
		if (isValidNote(note)) {
			encodedVelocities_[static_cast<std::size_t>(note)] = 0;
		}
	}

	[[nodiscard]] bool contains(int32_t note) const {
		return isValidNote(note) && encodedVelocities_[static_cast<std::size_t>(note)] != 0;
	}

	[[nodiscard]] bool empty() const {
		for (uint8_t encodedVelocity : encodedVelocities_) {
			if (encodedVelocity != 0) {
				return false;
			}
		}
		return true;
	}

	void clear() { encodedVelocities_.fill(0); }

	template <typename ReleaseNote>
	void releaseAll(ReleaseNote&& releaseNote) {
		for (int32_t note = 0; note < kNoteCount; ++note) {
			uint8_t& encodedVelocity = encodedVelocities_[static_cast<std::size_t>(note)];
			if (encodedVelocity == 0) {
				continue;
			}

			const uint8_t releaseVelocity = encodedVelocity - 1;
			encodedVelocity = 0;
			releaseNote(note, releaseVelocity);
		}
	}

private:
	[[nodiscard]] static constexpr bool isValidNote(int32_t note) { return note >= 0 && note < kNoteCount; }

	std::array<uint8_t, kNoteCount> encodedVelocities_{};
};

static_assert(sizeof(SustainDeferredNotes) == SustainDeferredNotes::kNoteCount);
static_assert(std::is_trivially_copyable_v<SustainDeferredNotes>);

} // namespace deluge::midi_support
