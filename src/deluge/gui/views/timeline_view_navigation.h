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

#include <algorithm>
#include <cstdint>
#include <limits>

namespace deluge::gui::timeline_view_navigation {

[[nodiscard]] constexpr int32_t clampScroll(int64_t requested, int32_t minimum) {
	return static_cast<int32_t>(std::clamp<int64_t>(requested, minimum, std::numeric_limits<int32_t>::max()));
}

[[nodiscard]] constexpr int32_t floorToMultiple(int32_t position, uint32_t span) {
	if (span == 0) {
		return position;
	}

	const int64_t widePosition = position;
	const int64_t wideSpan = span;
	const int64_t quotient = widePosition / wideSpan;
	const int64_t remainder = widePosition % wideSpan;
	const int64_t floored = (remainder < 0 ? quotient - 1 : quotient) * wideSpan;
	return static_cast<int32_t>(
	    std::clamp<int64_t>(floored, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()));
}

[[nodiscard]] constexpr int32_t alignScrollForZoom(int32_t position, uint32_t span, int32_t minimum) {
	return std::max(floorToMultiple(position, span), minimum);
}

[[nodiscard]] constexpr int32_t reanchorScrollAfterStartEdit(int32_t oldScroll, int32_t oldLength, int32_t newLength,
                                                             int32_t minimum) {
	return clampScroll(static_cast<int64_t>(oldScroll) + newLength - oldLength, minimum);
}

} // namespace deluge::gui::timeline_view_navigation
