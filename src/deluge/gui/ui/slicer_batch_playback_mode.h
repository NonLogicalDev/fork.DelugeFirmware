/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "definitions_cxx.hpp"
#include <cstdint>

namespace deluge::gui::slicer_playback {

enum class BatchMode : uint8_t {
	AUTO,
	CUT,
	ONCE,
};

constexpr int32_t toMenuIndex(BatchMode mode) {
	return static_cast<int32_t>(mode);
}

constexpr bool setFromMenuIndex(BatchMode& mode, int32_t menuIndex) {
	switch (menuIndex) {
	case 0:
		mode = BatchMode::AUTO;
		return true;
	case 1:
		mode = BatchMode::CUT;
		return true;
	case 2:
		mode = BatchMode::ONCE;
		return true;
	default:
		return false;
	}
}

constexpr SampleRepeatMode resolve(BatchMode mode, uint32_t lengthMSPerSlice, SampleRepeatMode configuredDefault) {
	switch (mode) {
	case BatchMode::AUTO:
		return lengthMSPerSlice < 2002 ? SampleRepeatMode::ONCE : configuredDefault;
	case BatchMode::CUT:
		return SampleRepeatMode::CUT;
	case BatchMode::ONCE:
		return SampleRepeatMode::ONCE;
	}

	return configuredDefault;
}

} // namespace deluge::gui::slicer_playback
