/*
 * Copyright © 2016-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

namespace deluge::gui::ui::keyboard {

enum class VelocityDrumsProfile : uint8_t {
	Full = 0,
	Four,
	Two,
	Fixed,
};

inline constexpr uint8_t kDefaultVelocityDrumsFixedVelocity = 64;

constexpr bool isValidVelocityDrumsProfile(int32_t rawValue) {
	return rawValue >= static_cast<int32_t>(VelocityDrumsProfile::Full)
	       && rawValue <= static_cast<int32_t>(VelocityDrumsProfile::Fixed);
}

constexpr VelocityDrumsProfile velocityDrumsProfileFromValue(int32_t rawValue) {
	return isValidVelocityDrumsProfile(rawValue) ? static_cast<VelocityDrumsProfile>(rawValue)
	                                             : VelocityDrumsProfile::Full;
}

constexpr bool isValidVelocityDrumsFixedVelocity(int32_t rawValue) {
	return rawValue >= 1 && rawValue <= 127;
}

constexpr uint8_t velocityDrumsFixedVelocityFromValue(int32_t rawValue) {
	return isValidVelocityDrumsFixedVelocity(rawValue) ? static_cast<uint8_t>(rawValue)
	                                                   : kDefaultVelocityDrumsFixedVelocity;
}

constexpr uint8_t clampVelocityDrumsFixedVelocity(int32_t rawValue) {
	return static_cast<uint8_t>(rawValue < 1 ? 1 : rawValue > 127 ? 127 : rawValue);
}

constexpr uint8_t velocityFromOrderedDrumsRegions(uint32_t coordinate, uint32_t length, uint32_t maxRegions) {
	if (length == 0 || maxRegions == 0) {
		return 0;
	}

	uint32_t regionCount = length < maxRegions ? length : maxRegions;
	uint32_t boundedCoordinate = coordinate < length ? coordinate : length - 1;
	uint32_t region = boundedCoordinate * regionCount / length;
	return static_cast<uint8_t>((127 * (region + 1) + regionCount - 1) / regionCount);
}

constexpr uint8_t velocityFromDrumsProfile(VelocityDrumsProfile profile, uint32_t localX, uint32_t localY,
                                           uint32_t blockWidth, uint32_t blockHeight, uint8_t fixedVelocity,
                                           uint8_t fullVelocity) {
	if (profile == VelocityDrumsProfile::Full || blockWidth == 0 || blockHeight == 0) {
		return fullVelocity;
	}

	uint8_t safeFixedVelocity = velocityDrumsFixedVelocityFromValue(fixedVelocity);
	if (profile == VelocityDrumsProfile::Fixed || (blockWidth == 1 && blockHeight == 1)) {
		return safeFixedVelocity;
	}

	if (profile == VelocityDrumsProfile::Two) {
		if (blockHeight > 1) {
			return velocityFromOrderedDrumsRegions(localY, blockHeight, 2);
		}
		return velocityFromOrderedDrumsRegions(localX, blockWidth, 2);
	}

	if (profile == VelocityDrumsProfile::Four) {
		if (blockWidth > 1 && blockHeight > 1) {
			uint32_t boundedX = localX < blockWidth ? localX : blockWidth - 1;
			uint32_t boundedY = localY < blockHeight ? localY : blockHeight - 1;
			uint32_t quadrant = (boundedY * 2 / blockHeight) * 2 + (boundedX * 2 / blockWidth);
			return quadrant == 3 ? 127 : static_cast<uint8_t>((quadrant + 1) * 32);
		}
		if (blockHeight > 1) {
			return velocityFromOrderedDrumsRegions(localY, blockHeight, 4);
		}
		return velocityFromOrderedDrumsRegions(localX, blockWidth, 4);
	}

	return fullVelocity;
}

constexpr uint32_t velocityDrumsIntensityIndex(uint8_t velocity, uint32_t cellCount) {
	if (cellCount <= 1) {
		return 0;
	}
	return static_cast<uint32_t>((static_cast<uint64_t>(velocity) * (cellCount - 1) + 63) / 127);
}

} // namespace deluge::gui::ui::keyboard
