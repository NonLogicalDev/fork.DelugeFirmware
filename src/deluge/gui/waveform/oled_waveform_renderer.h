/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gui/waveform/oled_waveform_render_data.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace deluge::gui::waveform {

namespace detail {

constexpr uint32_t kOledWaveformScaleBits = 40;

/// Apply a precomputed fixed-point scale, then correct its possible one-unit underestimate.
/// Input spans are bounded to 2^32, so Q40 reciprocal error is less than 1 / 256 of an output unit.
[[nodiscard]] constexpr uint64_t scaleWithExactFloor(uint64_t offset, uint64_t inputSpan, uint64_t outputSpan,
                                                     uint64_t scaleQ40) {
	uint64_t scaled = (offset * scaleQ40) >> kOledWaveformScaleBits;
	if (offset * outputSpan >= (scaled + 1) * inputSpan) {
		scaled++;
	}
	return scaled;
}

[[nodiscard]] constexpr uint64_t scaleViewportOffset(uint64_t offset, uint64_t inputSpan, uint64_t outputSpan) {
	if (inputSpan == 0 || outputSpan == 0 || offset >= inputSpan) {
		return 0;
	}
	const uint64_t quotient = inputSpan / outputSpan;
	const uint64_t remainder = inputSpan % outputSpan;
	uint64_t low = 0;
	uint64_t high = outputSpan - 1;
	while (low < high) {
		const uint64_t candidate = low + ((high - low + 1) >> 1);
		const uint64_t remainderProduct = remainder * candidate;
		const uint64_t boundary =
		    quotient * candidate + remainderProduct / outputSpan + (remainderProduct % outputSpan != 0);
		if (boundary <= offset) {
			low = candidate;
		}
		else {
			high = candidate - 1;
		}
	}
	return low;
}

template <typename Canvas>
void renderOledWaveformContourData(Canvas& canvas, int32_t const* minima, int32_t const* maxima,
                                   uint8_t const* statuses, size_t bucketCount, int32_t minValue, int32_t maxValue,
                                   int32_t top, int32_t bottom, bool reversed) {
	const int32_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / bucketCount;

	top = std::max<int32_t>(top, OLED_MAIN_TOPMOST_PIXEL);
	bottom = std::min<int32_t>(bottom, OLED_MAIN_HEIGHT_PIXELS - 1);
	if (top > bottom || minValue > maxValue) {
		return;
	}

	const bool flat = minValue == maxValue;
	const uint64_t valueSpan = flat ? 0 : static_cast<uint64_t>(static_cast<int64_t>(maxValue) - minValue);
	const uint64_t plotHeight = static_cast<uint64_t>(bottom - top);
	const uint64_t scaleQ40 = flat ? 0 : (plotHeight << kOledWaveformScaleBits) / valueSpan;

	auto mapToY = [=](int32_t value) {
		if (flat) {
			return top + static_cast<int32_t>(plotHeight >> 1);
		}
		if (value <= minValue) {
			return bottom;
		}
		if (value >= maxValue) {
			return top;
		}

		const uint64_t offset = static_cast<uint64_t>(static_cast<int64_t>(value) - minValue);
		const uint64_t scaled = scaleWithExactFloor(offset, valueSpan, plotHeight, scaleQ40);
		return bottom - static_cast<int32_t>(scaled);
	};

	for (size_t outputBucket = 0; outputBucket < bucketCount; outputBucket++) {
		const size_t sourceBucket = reversed ? (bucketCount - 1 - outputBucket) : outputBucket;
		if (statuses[sourceBucket] != COL_STATUS_INVESTIGATED) {
			continue;
		}

		const int32_t minY = mapToY(minima[sourceBucket]);
		const int32_t maxY = mapToY(maxima[sourceBucket]);
		const int32_t startX = outputBucket * pixelsPerBucket;
		const int32_t endX = startX + pixelsPerBucket - 1;

		canvas.drawHorizontalLine(minY, startX, endX);
		if (maxY != minY) {
			canvas.drawHorizontalLine(maxY, startX, endX);
		}
	}
}

} // namespace detail

/// Maps positions in the cached sample viewport to the 128-pixel OLED width.
class OledWaveformViewport {
public:
	template <typename RenderData>
	explicit constexpr OledWaveformViewport(RenderData const& data) : OledWaveformViewport(data.xScroll, data.xZoom) {}

	explicit constexpr OledWaveformViewport(int64_t scroll, int64_t zoom) : scroll_(scroll) {
		if (zoom <= 0) {
			return;
		}

		const uint64_t unsignedZoom = static_cast<uint64_t>(zoom);
		if (unsignedZoom > std::numeric_limits<uint64_t>::max() / kDisplayWidth) {
			return;
		}

		span_ = unsignedZoom * kDisplayWidth;
	}

	/// Return an OLED x coordinate, or -1 when the requested boundary is outside this viewport.
	/// End boundaries are exclusive, so the viewport end maps to its final visible pixel.
	[[nodiscard]] constexpr int32_t samplePositionToX(int64_t position, bool endBoundary = false) const {
		if (span_ == 0) {
			return -1;
		}

		if (endBoundary) {
			if (position == std::numeric_limits<int64_t>::min()) {
				return -1;
			}
			position--;
		}

		if (position < scroll_) {
			return -1;
		}

		const uint64_t offset = static_cast<uint64_t>(position) - static_cast<uint64_t>(scroll_);
		if (offset >= span_) {
			return -1;
		}

		return static_cast<int32_t>(detail::scaleViewportOffset(offset, span_, OLED_MAIN_WIDTH_PIXELS));
	}

	[[nodiscard]] constexpr uint64_t span() const { return span_; }

private:
	int64_t scroll_ = 0;
	uint64_t span_ = 0;
};

/// Draw the cached pad waveform as a sparse, two-line OLED contour.
///
/// This function deliberately accepts no Sample: OLED rendering must not load or inspect audio data. Bins not yet
/// investigated are left blank.
template <typename Canvas>
void renderOledWaveformContour(Canvas& canvas, WaveformRenderData const& data, int32_t minValue, int32_t maxValue,
                               int32_t top, int32_t bottom, bool reversed = false) {
	static_assert(OLED_MAIN_WIDTH_PIXELS % kDisplayWidth == 0);
	detail::renderOledWaveformContourData(canvas, data.minPerCol, data.maxPerCol, data.colStatus, kDisplayWidth,
	                                      minValue, maxValue, top, bottom, reversed);
}

template <typename Canvas>
void renderOledWaveformContour(Canvas& canvas, OledWaveformRenderData const& data, int32_t minValue, int32_t maxValue,
                               int32_t top, int32_t bottom, bool reversed = false) {
	static_assert(OLED_MAIN_WIDTH_PIXELS % kOledWaveformBucketCount == 0);
	detail::renderOledWaveformContourData(canvas, data.minPerCol, data.maxPerCol, data.colStatus,
	                                      kOledWaveformBucketCount, minValue, maxValue, top, bottom, reversed);
}

} // namespace deluge::gui::waveform
