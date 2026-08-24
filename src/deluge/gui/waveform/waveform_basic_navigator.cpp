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

#include "gui/waveform/waveform_basic_navigator.h"
#include "definitions_cxx.hpp"
#include "gui/ui/ui.h"
#include "gui/waveform/waveform_renderer.h"
#include "hid/led/pad_leds.h"
#include "model/sample/sample.h"
#include "storage/multi_range/multisample_range.h"
#include "util/misc.h"
#include <algorithm>
#include <limits>

WaveformBasicNavigator waveformBasicNavigator{};

WaveformBasicNavigator::WaveformBasicNavigator() {
}

void WaveformBasicNavigator::opened(SampleHolder* holder) {
	// Only if range is provided, grabs navigation from that if possible

	renderData.xScroll = -1;
	oledRenderSample = nullptr;
	oledRenderDataComplete = false;
	oledPadRenderDataCurrent = false;
	oledRenderData.xScroll = -1;

	// If want to use saved pos and sample already has peak info stored...
	if (holder && holder->waveformViewZoom) {
		xScroll = holder->waveformViewScroll;
		xZoom = holder->waveformViewZoom;

		potentiallyAdjustScrollPosition();
	}
	else {
		xScroll = 0;
		xZoom = getMaxZoom();
	}
}

int64_t WaveformBasicNavigator::getMaxZoom() {
	if (sample->lengthInSamples == 0) {
		return 1;
	}
	const uint64_t maxZoom = ((sample->lengthInSamples - 1) >> kDisplayWidthMagnitude) + 1;
	return static_cast<int64_t>(
	    std::min<uint64_t>(maxZoom, static_cast<uint64_t>(std::numeric_limits<int64_t>::max())));
}

bool WaveformBasicNavigator::zoom(int32_t offset, bool shouldAllowExtraScrollRight, MarkerColumn* cols,
                                  MarkerType markerType, bool useOledWaveformCache) {
	int64_t oldScroll = xScroll;
	int64_t oldZoom = xZoom;

	int64_t newXZoom;

	// In
	if (offset >= 0) {
		if (xZoom < 2) {
			return false;
		}

		bool isSquareNumber = false;
		int64_t nextSquareNumber = 0;
		for (int32_t i = 0; i < 63; i++) {
			const int64_t squareNumber = int64_t{1} << i;
			if (squareNumber == xZoom) {
				isSquareNumber = true;
				break;
			}
			else if (squareNumber > xZoom) {
				nextSquareNumber = squareNumber;
				break;
			}
		}

		if (!isSquareNumber && nextSquareNumber != 0) {
			if (xZoom >= nextSquareNumber * 0.707) {
				newXZoom = nextSquareNumber >> 1;
			}
			else {
				newXZoom = nextSquareNumber >> 2;
			}
		}

		else {
			newXZoom = xZoom >> 1;
		}
	}

	// Out
	else {
		int64_t limit = getMaxZoom();
		if (xZoom >= limit) {
			return false;
		}
		newXZoom = (xZoom > limit / 2) ? limit : xZoom * 2;
		if (newXZoom >= limit || static_cast<double>(newXZoom) * 1.414 >= static_cast<double>(limit)) {
			newXZoom = limit;
		}
	}

	int32_t pinMarkerCol = -1;
	int64_t pinMarkerPos = xScroll + xZoom * (kDisplayWidth >> 1);
	MarkerType pinnedToMarkerType = MarkerType::NONE;

	if (markerType != MarkerType::NONE) {

		bool foundActiveMarker = false;

		for (int32_t m = 0; m < kNumMarkerTypes; m++) {
			int32_t col = cols[m].colOnScreen;

			if (col >= 0 && col < kDisplayWidth) {

				if (m == util::to_underlying(markerType)) {
bestYet:
					pinMarkerCol = col;
					if (static_cast<MarkerType>(m) >= MarkerType::LOOP_END) {
						pinMarkerCol++; // Pin to right-hand edge of end-marker's col
					}
					pinMarkerPos = cols[m].pos;
					pinnedToMarkerType = static_cast<MarkerType>(m);
				}
				else {
					int32_t pinMarkerDistanceFromCentre = pinMarkerCol - (kDisplayWidth >> 1);
					if (pinMarkerDistanceFromCentre < 0) {
						pinMarkerDistanceFromCentre = -pinMarkerDistanceFromCentre;
					}

					int32_t thisMarkerDistanceFromCentre = col - (kDisplayWidth >> 1);
					if (thisMarkerDistanceFromCentre < 0) {
						thisMarkerDistanceFromCentre = -thisMarkerDistanceFromCentre;
					}

					if (thisMarkerDistanceFromCentre < pinMarkerDistanceFromCentre) {
						goto bestYet;
					}
				}

				if (m == util::to_underlying(markerType)) {
					break;
				}
			}
		}

		// If marker on-screen...
		// if (pinMarkerCol >= 0) {
		// xScroll = pinMarkerPos - newXZoom * pinMarkerCol;
		//}
	}

	if (pinMarkerCol == -1) {
		pinMarkerCol = (kDisplayWidth >> 1);
	}

	xScroll = pinMarkerPos - newXZoom * pinMarkerCol;

	xZoom = newXZoom;

	// Make sure scroll is multiple of zoom
	if (pinnedToMarkerType >= MarkerType::LOOP_END) {
		xScroll = ((xScroll - 1) / xZoom + 1) * xZoom;
	}
	else {
		xScroll = xScroll / xZoom * xZoom;
	}

	potentiallyAdjustScrollPosition(shouldAllowExtraScrollRight);

	memcpy(PadLEDs::imageStore[(offset > 0) ? kDisplayHeight : 0], PadLEDs::image,
	       (kDisplayWidth + kSideBarWidth) * kDisplayHeight * sizeof(RGB));

	// Calculate pin squares
	int64_t pinNumerator = oldScroll - xScroll;
	int64_t pinDenominator = newXZoom - oldZoom;
	while (pinNumerator > std::numeric_limits<int64_t>::max() / 65536
	       || pinNumerator < std::numeric_limits<int64_t>::min() / 65536) {
		pinNumerator /= 2;
		pinDenominator /= 2;
	}
	int32_t zoomPinSquareBig = static_cast<int32_t>((pinNumerator * 65536) / pinDenominator);
	for (int32_t i = 0; i < kDisplayHeight; i++) {
		PadLEDs::zoomPinSquare[i] = zoomPinSquareBig;
		PadLEDs::transitionTakingPlaceOnRow[i] = true;
	}

	int32_t storeOffset = (offset > 0) ? 0 : kDisplayHeight;

	PadLEDs::clearTickSquares(false); // We were mostly fine without this here, but putting it here fixed weird problem
	                                  // where tick squares would
	// appear when zooming into waveform in SampleBrowser

	if (useOledWaveformCache) {
		prepareOledWaveformForPadRendering();
		waveformRenderer.renderFullScreenFromData(sample, &PadLEDs::imageStore[storeOffset], &renderData);
	}
	else {
		waveformRenderer.renderFullScreen(sample, xScroll, xZoom, &PadLEDs::imageStore[storeOffset], &renderData);
	}

	PadLEDs::zoomingIn = (offset > 0);
	PadLEDs::zoomMagnitude = PadLEDs::zoomingIn ? offset : -offset;

	currentUIMode |= UI_MODE_HORIZONTAL_ZOOM;
	PadLEDs::recordTransitionBegin(kZoomSpeed);
	PadLEDs::renderZoom();

	return true;
}

bool WaveformBasicNavigator::scroll(int32_t offset, bool shouldAllowExtraScrollRight, MarkerColumn* cols) {
	// Right
	if (offset >= 0) {

		if (shouldAllowExtraScrollRight) {
			if (xZoom > std::numeric_limits<int64_t>::max() - xScroll) {
				return false;
			}
			xScroll += xZoom;
		}
		else {
			const uint64_t scroll = static_cast<uint64_t>(std::max<int64_t>(xScroll, 0));
			const uint64_t zoom = static_cast<uint64_t>(xZoom);
			const bool viewportOverflows = zoom > (std::numeric_limits<uint64_t>::max() - scroll) / kDisplayWidth;
			if ((viewportOverflows || scroll + zoom * kDisplayWidth >= sample->lengthInSamples)
			    && (!cols || cols[util::to_underlying(MarkerType::END)].colOnScreen < kDisplayWidth)) {
				return false;
			}
			if (xZoom > std::numeric_limits<int64_t>::max() - xScroll) {
				return false;
			}
			xScroll += xZoom;
		}
	}

	// Left
	else {
		if (xScroll <= 0) {
			return false;
		}
		else if (xScroll < xZoom) {
			xScroll = 0;
		}
		xScroll -= xZoom;
	}

	return true;
}

bool WaveformBasicNavigator::isZoomedIn() {
	return (xZoom != getMaxZoom());
}

void WaveformBasicNavigator::potentiallyAdjustScrollPosition(bool shouldAllowExtraScrollRight) {
	// Make sure not scrolled too far left
	if (xScroll < 0) {
		xScroll = 0;
	}
	else {
		if (!shouldAllowExtraScrollRight) {
			// Make sure not scrolled too far right
			const uint64_t lengthInSamples = sample->lengthInSamples;
			const uint64_t zoom = static_cast<uint64_t>(xZoom);
			const uint64_t columns = lengthInSamples == 0 ? 0 : ((lengthInSamples - 1) / zoom) + 1;
			const uint64_t scrollLimitWide = columns <= kDisplayWidth ? 0 : (columns - kDisplayWidth) * zoom;
			const int64_t scrollLimit = static_cast<int64_t>(
			    std::min<uint64_t>(scrollLimitWide, static_cast<uint64_t>(std::numeric_limits<int64_t>::max())));
			if (xScroll > scrollLimit) {
				xScroll = scrollLimit;
			}
		}
	}
}

deluge::gui::waveform::OledWaveformPrepareResult WaveformBasicNavigator::prepareOledWaveformForPadRendering() {
	if (isOledWaveformCacheComplete()) {
		if (!oledPadRenderDataCurrent) {
			deluge::gui::waveform::aggregateOledWaveformToPadColumns(oledRenderData, renderData);
			oledPadRenderDataCurrent = true;
		}
		return {true, false};
	}

	const bool cacheWasCurrent = isOledWaveformCacheCurrent();
	const size_t investigatedBefore =
	    cacheWasCurrent ? deluge::gui::waveform::oledWaveformInvestigatedBucketCount(oledRenderData) : 0;
	if (!cacheWasCurrent) {
		if (oledRenderSample != sample) {
			oledRenderData.xScroll = -1;
		}
		oledRenderSample = sample;
		oledRenderDataComplete = false;
	}
	oledRenderDataComplete =
	    waveformRenderer.findPeaksPerOledBucket(sample, xScroll, static_cast<uint64_t>(xZoom), &oledRenderData);
	deluge::gui::waveform::aggregateOledWaveformToPadColumns(oledRenderData, renderData);
	oledPadRenderDataCurrent = true;
	const size_t investigatedAfter = deluge::gui::waveform::oledWaveformInvestigatedBucketCount(oledRenderData);
	return deluge::gui::waveform::oledWaveformPrepareResult(cacheWasCurrent, investigatedBefore, investigatedAfter,
	                                                        oledRenderDataComplete);
}

bool WaveformBasicNavigator::isOledWaveformCacheCurrent() const {
	return oledRenderSample == sample && oledRenderData.xScroll == xScroll && oledRenderData.xZoom == xZoom;
}

bool WaveformBasicNavigator::isOledWaveformCacheComplete() const {
	return isOledWaveformCacheCurrent() && oledRenderDataComplete;
}

bool WaveformBasicNavigator::hasAnyCurrentOledWaveformPeak() const {
	if (!isOledWaveformCacheCurrent()) {
		return false;
	}
	for (size_t bucket = 0; bucket < deluge::gui::waveform::kOledWaveformBucketCount; bucket++) {
		if (oledRenderData.colStatus[bucket] == COL_STATUS_INVESTIGATED) {
			return true;
		}
	}
	return false;
}

bool WaveformBasicNavigator::isPadWaveformCacheCurrent() const {
	return renderData.xScroll == xScroll && renderData.xZoom == xZoom;
}
