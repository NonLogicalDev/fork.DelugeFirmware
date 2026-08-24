/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Deluge Firmware.
 *
 * The Synthstrom Deluge Firmware is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "model/output_colour.h"

namespace deluge::output_colour {

namespace {
constexpr int32_t kHueRange = 192;
}

int16_t step(int16_t colour, int32_t offset) {
	int32_t nextColour = (colour + offset) % kHueRange;
	if (nextColour < 0) {
		nextColour += kHueRange;
	}

	// Zero means "not assigned" on Output, so cross it without storing it as a visible hue.
	if (nextColour == 0) {
		nextColour = offset < 0 ? kHueRange - 1 : 1;
	}

	return nextColour;
}

int32_t audioClipHueOffset(int32_t encoderOffset) {
	return encoderOffset * -3;
}

} // namespace deluge::output_colour
