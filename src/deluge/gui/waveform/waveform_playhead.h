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

#include "definitions_cxx.hpp"
#include "gui/waveform/oled_waveform_renderer.h"

#include <cstdint>

class Sample;

namespace deluge::gui::waveform {

enum class PadWaveformIntensity : uint16_t {
	FULL = 256,
	PLAYHEAD_FOCUSED = 224,
};

[[nodiscard]] constexpr PadWaveformIntensity padWaveformIntensityForRenderTarget(bool isLivePadImage) {
	return isLivePadImage ? PadWaveformIntensity::PLAYHEAD_FOCUSED : PadWaveformIntensity::FULL;
}

[[nodiscard]] constexpr bool isWaveformPlayheadCandidate(bool hasExactHolder, bool centerPartActive,
                                                         bool hasVoiceSample) {
	return hasExactHolder && centerPartActive && hasVoiceSample;
}

[[nodiscard]] constexpr int32_t padWaveformBrightness(bool hasColour, PadWaveformIntensity intensity) {
	const int32_t establishedBrightness = hasColour ? 256 : 128;
	return establishedBrightness * static_cast<int32_t>(intensity) / 256;
}

constexpr uint32_t kOledWaveformPlayheadRefreshInterval = kSampleRate / 20;

struct OledWaveformPlayheadInput {
	Sample const* displayedSample{};
	Sample const* playingSample{};
	int64_t samplePosition{};
	int64_t xScroll{};
	int64_t xZoom{};
};

class OledWaveformPlayheadState {
public:
	void reset() { *this = {}; }

	[[nodiscard]] bool update(OledWaveformPlayheadInput const& input, uint32_t now) {
		Sample const* desiredSample = nullptr;
		int32_t desiredColumn = -1;
		int64_t desiredScroll = 0;
		int64_t desiredZoom = 0;
		if (input.displayedSample && input.playingSample == input.displayedSample) {
			desiredSample = input.displayedSample;
			desiredScroll = input.xScroll;
			desiredZoom = input.xZoom;
			desiredColumn = OledWaveformViewport{input.xScroll, input.xZoom}.samplePositionToX(input.samplePosition);
			if (desiredColumn < 0) {
				desiredSample = nullptr;
				desiredScroll = 0;
				desiredZoom = 0;
			}
		}

		const bool changed = !initialized_ || desiredSample != sample_ || desiredColumn != column_
		                     || (desiredSample && (desiredScroll != xScroll_ || desiredZoom != xZoom_));
		if (!changed) {
			return false;
		}
		if (hasRefreshed_ && now - lastRefreshTime_ < kOledWaveformPlayheadRefreshInterval) {
			return false;
		}

		initialized_ = true;
		sample_ = desiredSample;
		column_ = desiredColumn;
		xScroll_ = desiredScroll;
		xZoom_ = desiredZoom;
		if (!sample_ && !hasRefreshed_) {
			return false;
		}
		lastRefreshTime_ = now;
		hasRefreshed_ = true;
		return true;
	}

	[[nodiscard]] int32_t visibleColumn(Sample const* displayedSample, int64_t xScroll, int64_t xZoom) const {
		return initialized_ && sample_ && sample_ == displayedSample && xScroll_ == xScroll && xZoom_ == xZoom ? column_
		                                                                                                       : -1;
	}

private:
	Sample const* sample_{};
	int64_t xScroll_{};
	int64_t xZoom_{};
	int32_t column_{-1};
	uint32_t lastRefreshTime_{};
	bool initialized_{};
	bool hasRefreshed_{};
};

template <typename Canvas>
void renderOledWaveformPlayhead(Canvas& canvas, OledWaveformPlayheadState const& state, Sample const* displayedSample,
                                int64_t xScroll, int64_t xZoom, int32_t top, int32_t bottom) {
	const int32_t x = state.visibleColumn(displayedSample, xScroll, xZoom);
	if (x >= 0) {
		canvas.invertArea(x, 1, top, bottom);
	}
}

} // namespace deluge::gui::waveform
