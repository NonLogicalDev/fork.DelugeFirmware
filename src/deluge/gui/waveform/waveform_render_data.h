/*
 * Copyright © 2018-2023 Synthstrom Audible Limited
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

#include "definitions_cxx.hpp"
#include <cstdint>
#include <limits>

#define COL_STATUS_INVESTIGATED 1
#define COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM 2

struct WaveformRenderData {
	int64_t xScroll;
	int64_t xZoom;
	int32_t maxPerCol[kDisplayWidth];
	int32_t minPerCol[kDisplayWidth];
	uint8_t colStatus[kDisplayWidth];
};

namespace deluge::gui::waveform {

struct WaveformBytePosition {
	uint64_t absoluteByte = 0;
	bool valid = false;
};

struct WaveformClusterPosition {
	uint64_t clusterIndex = 0;
	uint32_t byteWithinCluster = 0;
	bool valid = false;
};

namespace detail {

struct CheckedSamplePosition {
	int64_t value = 0;
	bool valid = false;
};

[[nodiscard]] constexpr CheckedSamplePosition addSampleOffset(int64_t base, uint64_t offset) {
	if (base >= 0) {
		const uint64_t positiveBase = static_cast<uint64_t>(base);
		if (offset > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - positiveBase) {
			return {};
		}
		return {static_cast<int64_t>(positiveBase + offset), true};
	}

	const uint64_t magnitude = static_cast<uint64_t>(-(base + 1)) + 1;
	if (offset < magnitude) {
		const uint64_t remaining = magnitude - offset;
		if (remaining == uint64_t{1} << 63) {
			return {std::numeric_limits<int64_t>::min(), true};
		}
		return {-static_cast<int64_t>(remaining), true};
	}

	const uint64_t positiveResult = offset - magnitude;
	if (positiveResult > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
		return {};
	}
	return {static_cast<int64_t>(positiveResult), true};
}

} // namespace detail

[[nodiscard]] constexpr WaveformBytePosition
waveformSampleBytePosition(int64_t samplePosition, uint32_t bytesPerSampleFrame, uint32_t audioDataStartPosBytes) {
	if (samplePosition < 0 || bytesPerSampleFrame == 0) {
		return {};
	}

	const uint64_t position = static_cast<uint64_t>(samplePosition);
	if (position > (std::numeric_limits<uint64_t>::max() - audioDataStartPosBytes) / bytesPerSampleFrame) {
		return {};
	}
	return {position * bytesPerSampleFrame + audioDataStartPosBytes, true};
}

[[nodiscard]] constexpr WaveformClusterPosition waveformClusterPosition(uint64_t absoluteByte,
                                                                        uint32_t clusterSizeMagnitude) {
	if (clusterSizeMagnitude >= 32) {
		return {};
	}
	const uint64_t clusterSize = uint64_t{1} << clusterSizeMagnitude;
	return {absoluteByte >> clusterSizeMagnitude, static_cast<uint32_t>(absoluteByte & (clusterSize - 1)), true};
}

} // namespace deluge::gui::waveform
