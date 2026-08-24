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

#include "gui/waveform/waveform_render_data.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace deluge::gui::waveform {

#ifndef DELUGE_OLED_WAVEFORM_BUCKET_COUNT
#define DELUGE_OLED_WAVEFORM_BUCKET_COUNT OLED_MAIN_WIDTH_PIXELS
#endif

[[nodiscard]] constexpr bool oledWaveformBucketCountIsValid(size_t count) {
	return count >= kDisplayWidth && count <= OLED_MAIN_WIDTH_PIXELS && OLED_MAIN_WIDTH_PIXELS % count == 0;
}

inline constexpr size_t kOledWaveformBucketCount = DELUGE_OLED_WAVEFORM_BUCKET_COUNT;
static_assert(oledWaveformBucketCountIsValid(kOledWaveformBucketCount));

struct WaveformSampleRange {
	int64_t start = 0;
	int64_t end = 0;
	bool valid = false;
};

/// Fixed peak cache used only by sample-editing views on OLED hardware.
struct OledWaveformRenderData {
	int64_t xScroll;
	int64_t xZoom;
	int32_t maxPerCol[kOledWaveformBucketCount];
	int32_t minPerCol[kOledWaveformBucketCount];
	uint8_t colStatus[kOledWaveformBucketCount];
};

struct OledWaveformPrepareResult {
	bool complete;
	bool cacheChanged;
};

[[nodiscard]] constexpr OledWaveformPrepareResult
oledWaveformPrepareResult(bool cacheWasCurrent, size_t investigatedBefore, size_t investigatedAfter, bool complete) {
	return {complete, !cacheWasCurrent || investigatedAfter > investigatedBefore};
}

[[nodiscard]] constexpr bool oledWaveformCacheMatches(OledWaveformRenderData const& data, int64_t xScroll,
                                                      int64_t xZoom) {
	return data.xScroll == xScroll && data.xZoom == xZoom;
}

/// Return true when the viewport changed and cached bucket states were invalidated.
inline bool updateOledWaveformCacheViewport(OledWaveformRenderData& data, int64_t xScroll, int64_t xZoom) {
	if (oledWaveformCacheMatches(data, xScroll, xZoom)) {
		return false;
	}
	data.xScroll = xScroll;
	data.xZoom = xZoom;
	for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
		data.colStatus[bucket] = 0;
	}
	return true;
}

[[nodiscard]] inline size_t oledWaveformPendingBucketCount(OledWaveformRenderData const& data) {
	size_t pending = 0;
	for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
		pending += data.colStatus[bucket] == 0;
	}
	return pending;
}

[[nodiscard]] inline size_t oledWaveformInvestigatedBucketCount(OledWaveformRenderData const& data) {
	size_t investigated = 0;
	for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
		investigated += data.colStatus[bucket] == COL_STATUS_INVESTIGATED;
	}
	return investigated;
}

/// Divide the 16-pad viewport into independently measured OLED buckets.
///
/// A viewport at least as wide as the cache is partitioned into contiguous quotient/remainder ranges. At a tighter
/// zoom than that, each bucket explicitly owns one sample and adjacent buckets may therefore repeat that sample.
[[nodiscard]] constexpr WaveformSampleRange oledWaveformBucketSampleRange(int64_t xScroll, uint64_t xZoom,
                                                                          size_t bucket, size_t bucketCount) {
	if (!oledWaveformBucketCountIsValid(bucketCount) || bucket >= bucketCount || xZoom == 0
	    || xZoom > std::numeric_limits<uint64_t>::max() / kDisplayWidth) {
		return {};
	}

	const uint64_t viewportSpan = xZoom * kDisplayWidth;
	uint64_t startOffset;
	uint64_t endOffset;
	if (viewportSpan >= bucketCount) {
		const uint64_t quotient = viewportSpan / bucketCount;
		const uint64_t remainder = viewportSpan % bucketCount;
		auto boundaryOffset = [=](size_t boundary) {
			const uint64_t remainderProduct = remainder * boundary;
			return quotient * boundary + remainderProduct / bucketCount + (remainderProduct % bucketCount != 0);
		};
		// The display projects sample offsets with floor division, so its inverse bucket boundaries use ceiling
		// division.
		startOffset = boundaryOffset(bucket);
		endOffset = boundaryOffset(bucket + 1);
	}
	else {
		startOffset = (bucket * viewportSpan) / bucketCount;
		endOffset = startOffset + 1;
	}

	const detail::CheckedSamplePosition start = detail::addSampleOffset(xScroll, startOffset);
	const detail::CheckedSamplePosition end = detail::addSampleOffset(xScroll, endOffset);
	if (!start.valid || !end.valid) {
		return {};
	}
	return {start.value, end.value, true};
}

/// Fold the editor-local OLED cache into the established 16-column pad cache without inspecting the Sample again.
inline void aggregateOledWaveformToPadColumns(OledWaveformRenderData const& source, WaveformRenderData& destination) {
	destination.xScroll = source.xScroll;
	destination.xZoom = source.xZoom;

	for (size_t padColumn = 0; padColumn < kDisplayWidth; padColumn++) {
		size_t firstBucket = (padColumn * kOledWaveformBucketCount) / kDisplayWidth;
		size_t endBucket = ((padColumn + 1) * kOledWaveformBucketCount) / kDisplayWidth;

		bool pending = false;
		bool investigated = false;
		int32_t minimum = std::numeric_limits<int32_t>::max();
		int32_t maximum = std::numeric_limits<int32_t>::min();
		for (size_t bucket = firstBucket; bucket < endBucket; bucket++) {
			const uint8_t status = source.colStatus[bucket];
			if (status == 0) {
				pending = true;
			}
			else if (status == COL_STATUS_INVESTIGATED) {
				investigated = true;
				if (source.minPerCol[bucket] < minimum) {
					minimum = source.minPerCol[bucket];
				}
				if (source.maxPerCol[bucket] > maximum) {
					maximum = source.maxPerCol[bucket];
				}
			}
		}

		if (pending) {
			destination.colStatus[padColumn] = 0;
		}
		else if (investigated) {
			destination.minPerCol[padColumn] = minimum;
			destination.maxPerCol[padColumn] = maximum;
			destination.colStatus[padColumn] = COL_STATUS_INVESTIGATED;
		}
		else {
			destination.colStatus[padColumn] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
		}
	}
}

} // namespace deluge::gui::waveform
