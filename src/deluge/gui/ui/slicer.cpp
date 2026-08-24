/*
 * Copyright © 2017-2023 Synthstrom Audible Limited
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

#include "gui/ui/slicer.h"
#include "definitions_cxx.hpp"
#include "gui/colour/colour.h"
#include "gui/context_menu/slicer_playback_mode.h"
#include "gui/l10n/l10n.h"
#include "gui/ui/browser/sample_browser.h"
#include "gui/ui/sound_editor.h"
#include "gui/views/instrument_clip_view.h"
#include "gui/waveform/oled_waveform_renderer.h"
#include "gui/waveform/waveform_basic_navigator.h"
#include "gui/waveform/waveform_renderer.h"
#include "hid/buttons.h"
#include "hid/display/display.h"
#include "hid/display/oled.h"
#include "hid/led/pad_leds.h"
#include "hid/matrix/matrix_driver.h"
#include "memory/general_memory_allocator.h"
#include "model/action/action_logger.h"
#include "model/clip/instrument_clip.h"
#include "model/drum/drum.h"
#include "model/drum/generated_slice_name.h"
#include "model/instrument/kit.h"
#include "model/model_stack.h"
#include "model/note/note_row.h"
#include "model/sample/sample.h"
#include "model/song/song.h"
#include "model/voice/voice.h"
#include "model/voice/voice_sample.h"
#include "modulation/params/param_manager.h"
#include "modulation/params/param_set.h"
#include "processing/engines/audio_engine.h"
#include "processing/sound/sound.h"
#include "processing/sound/sound_drum.h"
#include "storage/flash_storage.h"
#include "storage/multi_range/multisample_range.h"
#include "util/functions.h"
#include <algorithm>
#include <cstring>

using namespace deluge::gui;

Slicer slicer{};

namespace params = deluge::modulation::params;

namespace {

constexpr int32_t kOledWaveformTop = OLED_MAIN_TOPMOST_PIXEL + kTextSpacingY + 2;
constexpr int32_t kOledWaveformBottom = OLED_MAIN_HEIGHT_PIXELS - 1;
constexpr uint64_t kMaxOledRegionBoundaries = OLED_MAIN_WIDTH_PIXELS / 2;

uint64_t divideRoundUp(uint64_t numerator, uint64_t denominator) {
	const uint64_t quotient = numerator / denominator;
	return quotient + (quotient * denominator != numerator);
}

void drawOledBoundary(deluge::hid::display::oled_canvas::Canvas& canvas, int32_t x, bool selected) {
	if (x < 0 || x >= OLED_MAIN_WIDTH_PIXELS) {
		return;
	}

	if (selected) {
		canvas.drawVerticalLine(x, kOledWaveformTop, kOledWaveformBottom);
	}
	else {
		canvas.drawVerticalLine(x, kOledWaveformTop, kOledWaveformTop + 2);
		canvas.drawVerticalLine(x, kOledWaveformBottom - 2, kOledWaveformBottom);
	}
}

int32_t getNextGeneratedSliceSeriesIndex(Kit* kit, SoundDrum* anchorDrum) {
	int32_t highestSeriesIndex = -1;
	for (Drum* drum = kit->firstDrum; drum; drum = drum->next) {
		if (drum->type != DrumType::SOUND || drum == anchorDrum) {
			continue;
		}

		int32_t seriesIndex = deluge::generated_slice_name::getSeriesIndex(drum->drumName);
		highestSeriesIndex = std::max(highestSeriesIndex, seriesIndex);
	}

	return highestSeriesIndex + 1;
}

} // namespace

bool Slicer::opened() {

	actionLogger.deleteAllLogs();

	numClips = 16;

	numManualSlice = 1;
	currentSlice = 0;
	slicerMode = requestedInitialMode;
	requestedInitialMode = SLICER_MODE_REGION;
	horizontalEncoderPressed = false;
	horizontalEncoderPressUsed = false;
	usesExistingKit = !sampleBrowser.canImportWholeKit();
	batchPlaybackMode = deluge::gui::slicer_playback::BatchMode::AUTO;
	manualPreviewChangedRepeatMode = false;
	for (int32_t i = 0; i < MAX_MANUAL_SLICES; i++) {
		manualSlicePoints[i].startPos = 0;
		manualSlicePoints[i].transpose = 0;
		manualSliceDrums[i] = nullptr;
	}

	focusRegained();
	return true;
}

void Slicer::focusRegained() {

	if (display->have7SEG()) {
		redraw();
	}

	uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
}

void Slicer::renderOLED(deluge::hid::display::oled_canvas::Canvas& canvas) {
	Sample* sample = waveformBasicNavigator.sample;
	char valueBuffer[24];

	if (slicerMode == SLICER_MODE_REGION) {
		canvas.drawString("Region", 0, OLED_MAIN_TOPMOST_PIXEL, kTextSpacingX, kTextSpacingY);
		intToString(numClips, valueBuffer);
	}
	else {
		canvas.drawString("Slice", 0, OLED_MAIN_TOPMOST_PIXEL, kTextSpacingX, kTextSpacingY);
		snprintf(valueBuffer, sizeof(valueBuffer), "%d/%d %d", currentSlice + 1, numManualSlice,
		         manualSlicePoints[currentSlice].startPos);
	}
	canvas.drawStringAlignRight(valueBuffer, OLED_MAIN_TOPMOST_PIXEL, kTextSpacingX, kTextSpacingY);

	if (!sample) {
		return;
	}

	if (waveformBasicNavigator.hasAnyCurrentOledWaveformPeak()) {
		deluge::gui::waveform::renderOledWaveformContour(canvas, waveformBasicNavigator.oledRenderData,
		                                                 sample->minValueFound, sample->maxValueFound, kOledWaveformTop,
		                                                 kOledWaveformBottom);
	}
	else if (waveformBasicNavigator.isPadWaveformCacheCurrent()) {
		deluge::gui::waveform::renderOledWaveformContour(canvas, waveformBasicNavigator.renderData,
		                                                 sample->minValueFound, sample->maxValueFound, kOledWaveformTop,
		                                                 kOledWaveformBottom);
	}
	deluge::gui::waveform::OledWaveformViewport viewport{waveformBasicNavigator.xScroll, waveformBasicNavigator.xZoom};

	if (slicerMode == SLICER_MODE_REGION) {
		const uint64_t sampleLength = sample->lengthInSamples;
		const uint64_t regionCount = numClips;
		if (sampleLength == 0 || regionCount < 2 || viewport.span() == 0) {
			return;
		}

		const uint64_t viewStart = std::max<int64_t>(waveformBasicNavigator.renderData.xScroll, 0);
		const uint64_t firstBoundary = std::max<uint64_t>(1, divideRoundUp(viewStart * regionCount, sampleLength));
		if (firstBoundary >= regionCount) {
			return;
		}
		const uint64_t boundaryStride = std::max<uint64_t>(
		    1, divideRoundUp(2 * regionCount * viewport.span(), sampleLength * OLED_MAIN_WIDTH_PIXELS));

		const uint64_t firstProduct = sampleLength * firstBoundary;
		uint64_t boundary = firstProduct / regionCount;
		uint64_t phase = firstProduct - boundary * regionCount;
		const uint64_t stepProduct = sampleLength * boundaryStride;
		const uint64_t boundaryStep = stepProduct / regionCount;
		const uint64_t phaseStep = stepProduct - boundaryStep * regionCount;
		const uint64_t viewEnd = viewStart + viewport.span();
		int32_t previousX = -2;
		uint64_t iterations = 0;
		for (uint64_t i = firstBoundary; i < regionCount && iterations < kMaxOledRegionBoundaries;
		     i += boundaryStride, iterations++) {
			if (boundary >= viewEnd) {
				break;
			}

			int32_t x = viewport.samplePositionToX(boundary);
			if (x >= 0 && x - previousX >= 2) {
				drawOledBoundary(canvas, x, false);
				previousX = x;
			}

			boundary += boundaryStep;
			phase += phaseStep;
			if (phase >= regionCount) {
				boundary++;
				phase -= regionCount;
			}
		}
	}
	else {
		for (int32_t i = 0; i < numManualSlice; i++) {
			int32_t x = viewport.samplePositionToX(manualSlicePoints[i].startPos);
			drawOledBoundary(canvas, x, i == currentSlice);
		}

		int64_t currentEnd = (currentSlice + 1 < numManualSlice) ? manualSlicePoints[currentSlice + 1].startPos
		                                                         : sample->lengthInSamples;
		int32_t endX = viewport.samplePositionToX(currentEnd, true);
		drawOledBoundary(canvas, endX, true);
	}
}

void Slicer::redraw() {
	display->setTextAsNumber(slicerMode == SLICER_MODE_REGION ? numClips : numManualSlice, 255, true);
}

bool Slicer::renderMainPads(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
                            uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea) {
	const bool useOledWaveformData = display->haveOLED() && image == PadLEDs::image;
	deluge::gui::waveform::OledWaveformPrepareResult oledWaveform{true, false};
	if (useOledWaveformData) {
		oledWaveform = waveformBasicNavigator.prepareOledWaveformForPadRendering();
	}

	if (slicerMode == SLICER_MODE_REGION) {
		if (useOledWaveformData) {
			waveformRenderer.renderFullScreenFromData(waveformBasicNavigator.sample, image,
			                                          &waveformBasicNavigator.renderData);
		}
		else {
			if (display->haveOLED()) {
				waveformBasicNavigator.invalidateOledPadRenderData();
			}
			waveformRenderer.renderFullScreen(waveformBasicNavigator.sample, waveformBasicNavigator.xScroll,
			                                  waveformBasicNavigator.xZoom, image, &waveformBasicNavigator.renderData);
		}
	}
	else if (slicerMode == SLICER_MODE_MANUAL) {

		RGB myImage[kDisplayHeight][kDisplayWidth + kSideBarWidth];
		if (useOledWaveformData) {
			waveformRenderer.renderFullScreenFromData(waveformBasicNavigator.sample, myImage,
			                                          &waveformBasicNavigator.renderData);
		}
		else {
			if (display->haveOLED()) {
				waveformBasicNavigator.invalidateOledPadRenderData();
			}
			waveformRenderer.renderFullScreen(waveformBasicNavigator.sample, waveformBasicNavigator.xScroll,
			                                  waveformBasicNavigator.xZoom, myImage,
			                                  &waveformBasicNavigator.renderData);
		}

		for (int32_t xx = 0; xx < kDisplayWidth; xx++) {
			for (int32_t yy = 0; yy < kDisplayHeight / 2; yy++) {
				image[yy + 4][xx] = RGB::average(myImage[yy * 2][xx], myImage[yy * 2 + 1][xx]);
			}
		}
		for (int32_t i = 0; i < numManualSlice; i++) { // Slices
			int32_t x = manualSlicePoints[i].startPos / (waveformBasicNavigator.sample->lengthInSamples + 0.0) * 16;
			image[4][x] = RGB{
			    1,
			    (i == currentSlice) ? 200_u8 : 16_u8,
			    1,
			};
		}

		for (int32_t i = 0; i < MAX_MANUAL_SLICES; i++) { // Lower screen
			int32_t xx = (i % 4) + (i / 16) * 4;
			int32_t yy = (i / 4) % 4;
			int32_t page = i / 16;

			RGB colour = RGB::monochrome(3);
			size_t dimLevel = (i < numManualSlice) ? 2 : 6;
			if (page % 2 == 0) {
				colour = colours::green.dim(dimLevel);
			}
			else {
				colour = colours::darkblue.dim(dimLevel);
			}
			if (i == this->currentSlice) {
				colour = colours::green.dim();
			}

			image[yy][xx] = colour;
		}
	}
	if (!oledWaveform.complete) {
		uiNeedsRendering(this, 0xFFFFFFFF, 0);
	}
	if (oledWaveform.cacheChanged) {
		renderUIsForOled();
	}
	return true;
}

bool Slicer::renderSidebar(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
                           uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth]) {
	if (!image) {
		return true;
	}

	instrumentClipView.renderSidebar(whichRows, image, occupancyMask);

	// The status column normally belongs to the view underneath. Slicer reserves its top three pads for the pending
	// batch mode while leaving all other sidebar controls available.
	constexpr RGB modeColours[] = {
	    colours::yellow_orange,
	    colours::red,
	    colours::magenta,
	};
	for (int32_t y = 0; y < 3; y++) {
		if (whichRows & (1 << y)) {
			RGB colour = modeColours[y];
			image[y][kDisplayWidth] = (y == (int32_t)batchPlaybackMode) ? colour : colour.dim(4);
			if (occupancyMask) {
				PadLEDs::refreshSidebarOccupancy(image[y], occupancyMask[y]);
			}
		}
	}

	return true;
}

const uint8_t zeroes[] = {0, 0, 0, 0, 0, 0, 0, 0};

void Slicer::graphicsRoutine() {

	int32_t newTickSquare = 255;
	VoiceSample* voiceSample = nullptr;
	SamplePlaybackGuide* guide = nullptr;

	MultisampleRange* range;
	SoundDrum* drum = (SoundDrum*)soundEditor.currentSound;

	if (getCurrentClip()->type == ClipType::INSTRUMENT && drum->hasActiveVoices()) {
		range = (MultisampleRange*)drum->sources[0].getOrCreateFirstRange();

		auto valid_voices_view = drum->voices() | std::views::filter([&](const Sound::ActiveVoice& voice) {
			                         // Ensure correct MultisampleRange.
			                         return voice->guides[0].audioFileHolder == range->getAudioFileHolder();
		                         });

		if (!valid_voices_view.empty()) {
			const Sound::ActiveVoice& voice = *std::ranges::max_element(valid_voices_view, {}, &Voice::orderSounded);

			VoiceUnisonPartSource* part = &voice->unisonParts[drum->numUnison >> 1].sources[0];
			if (part != nullptr && part->active) {
				voiceSample = part->voiceSample;
				guide = &voice->guides[soundEditor.currentSourceIndex];
			}
		}
	}

	if (voiceSample != nullptr) {
		int32_t samplePos = voiceSample->getPlaySample((Sample*)range->sampleHolder.audioFile, guide);
		if (samplePos >= waveformBasicNavigator.xScroll) {
			newTickSquare = (samplePos - waveformBasicNavigator.xScroll) / waveformBasicNavigator.xZoom;
			if (newTickSquare >= kDisplayWidth) {
				newTickSquare = 255;
			}
		}
	}

	uint8_t tickSquares[kDisplayHeight];
	memset(tickSquares, 255, kDisplayHeight);
	tickSquares[kDisplayHeight - 1] = newTickSquare;
	tickSquares[kDisplayHeight - 2] = newTickSquare;
	tickSquares[kDisplayHeight - 3] = newTickSquare;
	tickSquares[kDisplayHeight - 4] = newTickSquare;

	PadLEDs::setTickSquares(tickSquares, zeroes);
}

ActionResult Slicer::horizontalEncoderAction(int32_t offset) {
	if (horizontalEncoderPressed && offset != 0) {
		horizontalEncoderPressUsed = true;
	}

	if (slicerMode == SLICER_MODE_MANUAL) {
		const int32_t oldPos = manualSlicePoints[currentSlice].startPos;
		int32_t newPos = oldPos;
		newPos += (horizontalEncoderPressed ? 1000 : 100) * offset;

		if (currentSlice > 0 && newPos <= manualSlicePoints[currentSlice - 1].startPos + 1)
			newPos = manualSlicePoints[currentSlice - 1].startPos + 1;
		if (currentSlice < numManualSlice - 1 && newPos >= manualSlicePoints[currentSlice + 1].startPos - 1)
			newPos = manualSlicePoints[currentSlice + 1].startPos - 1;

		if (newPos < 0)
			newPos = 0;
		if (newPos > waveformBasicNavigator.sample->lengthInSamples)
			newPos = waveformBasicNavigator.sample->lengthInSamples;
		if (newPos == oldPos) {
			return ActionResult::DEALT_WITH;
		}
		manualSlicePoints[currentSlice].startPos = newPos;

		if (display->haveOLED()) {
			renderUIsForOled();
		}
		else {
			char buffer[12];
			strcpy(buffer, "");
			intToString(manualSlicePoints[currentSlice].startPos / 1000, buffer + strlen(buffer));
			display->displayPopup(buffer, 0, true);
		}
		uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
	}
	return ActionResult::DEALT_WITH;
}

ActionResult Slicer::verticalEncoderAction(int32_t offset, bool inCardRoutine) {
	if (slicerMode == SLICER_MODE_MANUAL) {

		manualSlicePoints[currentSlice].transpose += offset;
		if (manualSlicePoints[currentSlice].transpose > 24)
			manualSlicePoints[currentSlice].transpose = 24;
		if (manualSlicePoints[currentSlice].transpose < -24)
			manualSlicePoints[currentSlice].transpose = -24;
		if (display->haveOLED()) {
			char buffer[32];
			snprintf(buffer, 32, "Transpose: %d", manualSlicePoints[currentSlice].transpose);
			display->popupTextTemporary(buffer);
		}
		else {
			char buffer[12];
			strcpy(buffer, "");
			intToString(manualSlicePoints[currentSlice].transpose, buffer + strlen(buffer));
			display->displayPopup(buffer, 0, true);
		}
	}
	return ActionResult::DEALT_WITH;
}

void Slicer::selectEncoderAction(int8_t offset) {
	bool changed = false;
	if (slicerMode == SLICER_MODE_REGION) {
		const int32_t oldNumClips = numClips;
		numClips += offset;
		if (numClips == 257) {
			numClips = 2;
		}
		else if (numClips == 1) {
			numClips = 256;
		}
		changed = numClips != oldNumClips;
	}
	else { // SLICER_MODE_MANUAL
		const int32_t oldNumManualSlices = numManualSlice;
		const int32_t oldCurrentSlice = currentSlice;
		const int32_t oldFirstStart = manualSlicePoints[0].startPos;
		if (offset < 0) {
			numManualSlice += offset;
			if (numManualSlice <= 0) {
				numManualSlice = 1;
				manualSlicePoints[0].startPos = 0;
				manualSlicePoints[0].transpose = 0;
			}
		}
		if (currentSlice >= numManualSlice - 1) {
			currentSlice = numManualSlice - 1;
		}
		changed = numManualSlice != oldNumManualSlices || currentSlice != oldCurrentSlice
		          || manualSlicePoints[0].startPos != oldFirstStart;
		if (changed) {
			uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
		}
	}

	if (!changed) {
		return;
	}

	if (display->haveOLED()) {
		renderUIsForOled();
	}
	else {
		redraw();
	}
}

ActionResult Slicer::buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) {
	using namespace deluge::hid::button;

	// A bare Horizontal Encoder tap still switches Slicer mode. Turning it while held consumes that tap and provides
	// coarse Manual Slicer movement instead.
	if (b == X_ENC) {
		if (!on && inCardRoutine) {
			return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
		}
		if (on) {
			if (currentUIMode != UI_MODE_NONE) {
				return ActionResult::NOT_DEALT_WITH;
			}
			horizontalEncoderPressed = true;
			horizontalEncoderPressUsed = false;
			return ActionResult::DEALT_WITH;
		}

		if (!horizontalEncoderPressed) {
			return ActionResult::NOT_DEALT_WITH;
		}

		horizontalEncoderPressed = false;
		if (horizontalEncoderPressUsed) {
			horizontalEncoderPressUsed = false;
			return ActionResult::DEALT_WITH;
		}
		if (usesExistingKit) {
			return ActionResult::DEALT_WITH;
		}

		restoreManualPreviewRepeatMode();
		slicerMode++;
		slicerMode %= 2;
		if (slicerMode == SLICER_MODE_MANUAL)
			AudioEngine::stopAnyPreviewing();
		if (display->haveOLED()) {
			renderUIsForOled();
		}
		else {
			redraw();
		}

		((SoundDrum*)soundEditor.currentSound)->killAllVoices(); // stop
		uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
		return ActionResult::DEALT_WITH;
	}
	if (on && horizontalEncoderPressed) {
		horizontalEncoderPressUsed = true;
	}

	if (currentUIMode != UI_MODE_NONE || !on) {
		return ActionResult::NOT_DEALT_WITH;
	}

	if (b == BACK && Buttons::isShiftButtonPressed() && slicerMode == SLICER_MODE_MANUAL) {
		preview(0, 0, 0, 0);
		if (display->haveOLED()) {
			display->popupTextTemporary(deluge::l10n::get(deluge::l10n::String::STRING_FOR_STOPPED));
		}
		else {
			display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_STOPPED));
		}
		return ActionResult::DEALT_WITH;
	}
	// pop up Transpose value
	if (b == Y_ENC && on && slicerMode == SLICER_MODE_MANUAL && currentSlice < numManualSlice) {
		if (display->haveOLED()) {

			char buffer[24];
			strcpy(buffer, "Transpose: ");
			intToString(manualSlicePoints[currentSlice].transpose, buffer + strlen(buffer));
			display->popupTextTemporary(buffer);
		}
		else {
			char buffer[12];
			strcpy(buffer, "");
			intToString(manualSlicePoints[currentSlice].transpose, buffer + strlen(buffer));
			display->displayPopup(buffer, 0, true);
		}
		return ActionResult::DEALT_WITH;
	}

	// delete slice
	if (b == SAVE && on && slicerMode == SLICER_MODE_MANUAL) {
		int32_t xx = (currentSlice % 4) + (currentSlice / 16) * 4;
		int32_t yy = (currentSlice / 4) % 4;
		if (matrixDriver.isPadPressed(xx, yy) && currentSlice < numManualSlice) {
			int32_t target = currentSlice;

			for (int32_t i = 0; i < MAX_MANUAL_SLICES - 1; i++) {
				manualSlicePoints[i] = manualSlicePoints[(i >= target) ? i + 1 : i];
			}
			manualSlicePoints[MAX_MANUAL_SLICES - 1].startPos = 0;
			manualSlicePoints[MAX_MANUAL_SLICES - 1].transpose = 0;

			numManualSlice--;
			if (numManualSlice <= 0) {
				numManualSlice = 1;
				manualSlicePoints[0].startPos = 0;
				manualSlicePoints[0].transpose = 0;
			}
			if (currentSlice >= numManualSlice - 1) {
				currentSlice = numManualSlice - 1;
			}

			uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
			if (display->haveOLED()) {
				renderUIsForOled();
			}
			else {
				redraw();
			}
			return ActionResult::DEALT_WITH;
		}
	}

	if (b == SELECT_ENC) {
		if (inCardRoutine) {
			return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
		}
		deluge::gui::context_menu::slicerPlaybackMode.setupAndCheckAvailability();
		display->setNextTransitionDirection(1);
		openUI(&deluge::gui::context_menu::slicerPlaybackMode);
	}

	else if (b == BACK) {
		if (inCardRoutine) {
			return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
		}
		restoreManualPreviewRepeatMode();
		if (slicerMode == SLICER_MODE_MANUAL) {
			if (display->haveOLED()) {
				waveformBasicNavigator.prepareOledWaveformForPadRendering();
				waveformRenderer.renderFullScreenFromData(waveformBasicNavigator.sample, PadLEDs::image,
				                                          &waveformBasicNavigator.renderData);
			}
			else {
				waveformRenderer.renderFullScreen(waveformBasicNavigator.sample, waveformBasicNavigator.xScroll,
				                                  waveformBasicNavigator.xZoom, PadLEDs::image,
				                                  &waveformBasicNavigator.renderData);
			}
			SoundDrum* soundDrum = (SoundDrum*)soundEditor.currentSound;
			soundDrum->killAllVoices(); // stop
			MultisampleRange* range = (MultisampleRange*)soundDrum->sources[0].getOrCreateFirstRange();
			Sample* sample = (Sample*)range->sampleHolder.audioFile;
			range->sampleHolder.startPos = 0;
			range->sampleHolder.endPos = sample->lengthInSamples;
			range->sampleHolder.transpose = 0;
		}

		display->setNextTransitionDirection(-1);
		close();
	}
	else {
		return ActionResult::NOT_DEALT_WITH;
	}

	return ActionResult::DEALT_WITH;
}

int32_t Slicer::getBatchPlaybackModeMenuIndex() const {
	return deluge::gui::slicer_playback::toMenuIndex(batchPlaybackMode);
}

bool Slicer::confirmWithBatchPlaybackModeMenuIndex(int32_t menuIndex) {
	if (!deluge::gui::slicer_playback::setFromMenuIndex(batchPlaybackMode, menuIndex)) {
		return false;
	}

	bool confirmed = confirmSlices();
	if (!confirmed) {
		uiNeedsRendering(this, 0, 0xFFFFFFFF);
	}
	return confirmed;
}

bool Slicer::confirmSlices() {
	if (slicerMode == SLICER_MODE_REGION) {
		return doSlice();
	}

	SoundDrum* firstDrum = (SoundDrum*)soundEditor.currentSound;
	firstDrum->killAllVoices();
	numClips = numManualSlice;
	if (!doSlice()) {
		return false;
	}

	for (int32_t i = 0; i < numManualSlice; i++) {
		SoundDrum* soundDrum = manualSliceDrums[i];
		MultisampleRange* range = (MultisampleRange*)soundDrum->sources[0].getOrCreateFirstRange();
		range->sampleHolder.startPos = manualSlicePoints[i].startPos;
		range->sampleHolder.endPos = (i == numManualSlice - 1) ? waveformBasicNavigator.sample->lengthInSamples
		                                                       : manualSlicePoints[i + 1].startPos;
		range->sampleHolder.transpose = manualSlicePoints[i].transpose;
	}

	return true;
}

void Slicer::chooseBatchPlaybackMode(deluge::gui::slicer_playback::BatchMode newMode) {
	batchPlaybackMode = newMode;

	char const* popupText[2];
	switch (newMode) {
	case deluge::gui::slicer_playback::BatchMode::AUTO:
		popupText[0] = "DEF";
		popupText[1] = "Slice mode: Default";
		break;
	case deluge::gui::slicer_playback::BatchMode::CUT:
		popupText[0] = "CUT";
		popupText[1] = "Slice mode: All Cut";
		break;
	case deluge::gui::slicer_playback::BatchMode::ONCE:
		popupText[0] = "ONCE";
		popupText[1] = "Slice mode: All Once";
		break;
	}

	display->displayPopup(popupText);
	uiNeedsRendering(this, 0, 0xFFFFFFFF);
}

SampleRepeatMode Slicer::getBatchRepeatMode(uint32_t lengthMSPerSlice) const {
	return deluge::gui::slicer_playback::resolve(batchPlaybackMode, lengthMSPerSlice, FlashStorage::defaultSliceMode);
}

void Slicer::restoreManualPreviewRepeatMode() {
	if (manualPreviewChangedRepeatMode) {
		((SoundDrum*)soundEditor.currentSound)->sources[0].repeatMode = repeatModeBeforeManualPreview;
		manualPreviewChangedRepeatMode = false;
	}
}

void Slicer::stopAnyPreviewing() {
	SoundDrum* drum = (SoundDrum*)soundEditor.currentSound;
	drum->killAllVoices();
	if (drum->sources[0].ranges.getNumElements()) {
		MultisampleRange* range = (MultisampleRange*)drum->sources[0].ranges.getElement(0);
		range->sampleHolder.setAudioFile(nullptr);
	}
}
void Slicer::preview(int64_t startPoint, int64_t endPoint, int32_t transpose, int32_t on) {
	if (on) {
		SoundDrum* drum = (SoundDrum*)soundEditor.currentSound;

		char modelStackMemory[MODEL_STACK_MAX_SIZE];
		ModelStackWithThreeMainThings* modelStack = soundEditor.getCurrentModelStack(modelStackMemory);

		MultisampleRange* range = (MultisampleRange*)drum->sources[0].getOrCreateFirstRange();
		if (!manualPreviewChangedRepeatMode) {
			// Preview forces Once, but cancelling Slicer must leave the anchor's stored mode unchanged.
			repeatModeBeforeManualPreview = drum->sources[0].repeatMode;
			manualPreviewChangedRepeatMode = true;
		}
		drum->sources[0].repeatMode = SampleRepeatMode::ONCE;

		if (!waveformBasicNavigator.sample->filePath.equals(&range->sampleHolder.filePath)) {
			stopAnyPreviewing();
			range->sampleHolder.filePath.set(waveformBasicNavigator.sample->filePath.get());
			range->sampleHolder.loadFile(false, true, true);
		}
		range->sampleHolder.startPos = startPoint;
		if (endPoint != -1)
			range->sampleHolder.endPos = endPoint;
		range->sampleHolder.transpose = transpose;

		ParamCollectionSummary* summary = modelStack->paramManager->getPatchedParamSetSummary();
		ModelStackWithParamId* modelStackWithParamId =
		    modelStack->addParamCollectionAndId(summary->paramCollection, summary, params::LOCAL_ENV_0_RELEASE);
		ModelStackWithAutoParam* modelStackWithAutoParam =
		    modelStackWithParamId->paramCollection->getAutoParamFromId(modelStackWithParamId);
		modelStackWithAutoParam->autoParam->setCurrentValueWithNoReversionOrRecording(
		    modelStackWithAutoParam, getParamFromUserValue(params::LOCAL_ENV_0_RELEASE, 1));
		modelStackWithParamId =
		    modelStack->addParamCollectionAndId(summary->paramCollection, summary, params::LOCAL_ENV_0_ATTACK);
		modelStackWithAutoParam = modelStackWithParamId->paramCollection->getAutoParamFromId(modelStackWithParamId);
		modelStackWithAutoParam->autoParam->setCurrentValueWithNoReversionOrRecording(
		    modelStackWithAutoParam, getParamFromUserValue(params::LOCAL_ENV_0_ATTACK, 1));
	}
	instrumentClipView.sendAuditionNote(on, 0, 64, 0);
}

ActionResult Slicer::padAction(int32_t x, int32_t y, int32_t on) {
	if (on && horizontalEncoderPressed) {
		horizontalEncoderPressUsed = true;
	}
	if (x == kDisplayWidth && y < 3) {
		if (on) {
			deluge::gui::slicer_playback::BatchMode mode = batchPlaybackMode;
			if (deluge::gui::slicer_playback::setFromMenuIndex(mode, y)) {
				chooseBatchPlaybackMode(mode);
			}
		}
		return ActionResult::DEALT_WITH;
	}

	if (on && x < kDisplayWidth && y < kDisplayHeight / 2 && slicerMode == SLICER_MODE_MANUAL) { // pad on
		bool oledStateChanged = false;

		int32_t slicePadIndex = (x % 4 + (x / 4) * 16) + ((y % 4) * 4); //

		if (slicePadIndex < numManualSlice) { // play slice
			bool closePopup = (currentSlice != slicePadIndex);
			oledStateChanged = closePopup;
			currentSlice = slicePadIndex;
			if (slicePadIndex + 1 < numManualSlice) {
				preview(manualSlicePoints[slicePadIndex].startPos, manualSlicePoints[slicePadIndex + 1].startPos,
				        manualSlicePoints[slicePadIndex].transpose, on);
			}
			else if (slicePadIndex + 1 == numManualSlice) {
				preview(manualSlicePoints[slicePadIndex].startPos, waveformBasicNavigator.sample->lengthInSamples,
				        manualSlicePoints[slicePadIndex].transpose, on);
			}

			if (closePopup) {
				display->cancelPopup();
			}
		}
		else { // do slice

			VoiceSample* voiceSample = nullptr;
			SamplePlaybackGuide* guide = nullptr;
			MultisampleRange* range;
			SoundDrum* drum = (SoundDrum*)soundEditor.currentSound;

			if (getCurrentClip()->type == ClipType::INSTRUMENT && drum->hasActiveVoices()) {
				range = (MultisampleRange*)drum->sources[0].getOrCreateFirstRange();
				auto valid_voices_view = drum->voices() | std::views::filter([&](const Sound::ActiveVoice& voice) {
					                         // Ensure correct MultisampleRange.
					                         return voice->guides[0].audioFileHolder == range->getAudioFileHolder();
				                         });

				if (!valid_voices_view.empty()) {
					const Sound::ActiveVoice& assigned_voice =
					    *std::ranges::max_element(valid_voices_view, {}, &Voice::orderSounded);

					VoiceUnisonPartSource* part = &assigned_voice->unisonParts[drum->numUnison >> 1].sources[0];
					if (part != nullptr && part->active) {
						voiceSample = part->voiceSample;
						guide = &assigned_voice->guides[soundEditor.currentSourceIndex];
					}
				}
			}
			if (voiceSample != nullptr) {
				int32_t samplePos = voiceSample->getPlaySample((Sample*)range->sampleHolder.audioFile, guide);
				if (samplePos < waveformBasicNavigator.sample->lengthInSamples && numManualSlice < MAX_MANUAL_SLICES) {
					manualSlicePoints[numManualSlice].startPos = samplePos;
					manualSlicePoints[numManualSlice].transpose = 0;

					numManualSlice++;
					oledStateChanged = true;
					display->cancelPopup();

					SliceItem tmp;
					for (int32_t i = 0; i < (numManualSlice - 1); i++) {
						for (int32_t j = (numManualSlice - 1); j > i; j--) {
							if (manualSlicePoints[j].startPos < manualSlicePoints[j - 1].startPos) {
								tmp = manualSlicePoints[j];
								manualSlicePoints[j] = manualSlicePoints[j - 1];
								manualSlicePoints[j - 1] = tmp;
							}
						}
					}
				}
			}
		}

		if (display->haveOLED()) {
			if (oledStateChanged) {
				renderUIsForOled();
			}
		}
		else {
			redraw();
		}
		uiNeedsRendering(this, 0xFFFFFFFF, 0xFFFFFFFF);
	}
	else if (!on && x < kDisplayWidth && y < kDisplayHeight / 2 && slicerMode == SLICER_MODE_MANUAL) { // pad off
		preview(0, 0, 0, 0);                                                                           // off
	}

	if (slicerMode == SLICER_MODE_MANUAL) {
		return ActionResult::DEALT_WITH;
	}

	return sampleBrowser.padAction(x, y, on);
}

bool Slicer::doSlice() {

	AudioEngine::stopAnyPreviewing();
	bool isManualSlice = slicerMode == SLICER_MODE_MANUAL;
	int32_t firstDrumNoteRowIndex = 0;
	if (usesExistingKit
	    && !getCurrentInstrumentClip()->getNoteRowForDrum((SoundDrum*)soundEditor.currentSound,
	                                                      &firstDrumNoteRowIndex)) {
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_MANUAL_SLICE_NEEDS_TOP_EMPTY_KIT_PAD));
		return false;
	}

	Kit* kit = getCurrentKit();
	SoundDrum* firstDrum = (SoundDrum*)soundEditor.currentSound;
	int32_t generatedSliceSeriesIndex = getNextGeneratedSliceSeriesIndex(kit, firstDrum);

	Error error = sampleBrowser.claimAudioFileForInstrument();
	if (error != Error::NONE) {
getOut:
		display->displayError(error);
		return false;
	}

	MultisampleRange* firstRange = (MultisampleRange*)firstDrum->sources[0].getOrCreateFirstRange();
	if (!firstRange) {
		display->displayError(Error::INSUFFICIENT_RAM);
		return false;
	}

	Sample* sample = (Sample*)firstRange->sampleHolder.audioFile;
	if (!sample) {
		display->displayError(Error::FILE_UNREADABLE);
		return false;
	}

	// Do the first Drum

	// Ensure osc type is "sample"
	if (soundEditor.currentSource->oscType != OscType::SAMPLE) {
		soundEditor.currentSound->killAllVoices();
		soundEditor.currentSource->setOscType(OscType::SAMPLE);
	}

	char modelStackMemory[MODEL_STACK_MAX_SIZE];
	{
		ModelStackWithThreeMainThings* modelStack = soundEditor.getCurrentModelStack(modelStackMemory);
		ParamCollectionSummary* summary = modelStack->paramManager->getPatchedParamSetSummary();
		ParamSet* paramSet = (ParamSet*)summary->paramCollection;
		int32_t paramId = params::LOCAL_OSC_A_VOLUME + soundEditor.currentSourceIndex;
		ModelStackWithAutoParam* modelStackWithParam =
		    modelStack->addParam(paramSet, summary, paramId, &paramSet->params[paramId]);

		// Reset osc volume, if it's not automated
		if (!modelStackWithParam->autoParam->isAutomated()) {
			modelStackWithParam->autoParam->setCurrentValueWithNoReversionOrRecording(modelStackWithParam, 2147483647);
			//((ParamManagerBase*)soundEditor.currentParamManager)->setPatchedParamValue(params::LOCAL_OSC_A_VOLUME +
			// soundEditor.currentSourceIndex, 2147483647, 0xFFFFFFFF, 0, soundEditor.currentSound, currentSong,
			// getCurrentClip(), false);
		}

		if (isManualSlice) {
			manualSliceDrums[0] = firstDrum;
		}

		firstDrum->drumName = deluge::generated_slice_name::makeName(generatedSliceSeriesIndex, 1);
		firstDrum->nameIsDiscardable = false;

		uint32_t lengthInSamples = sample->lengthInSamples;

		uint32_t lengthSamplesPerSlice = lengthInSamples / numClips;
		uint32_t lengthMSPerSlice = lengthSamplesPerSlice * 1000 / sample->sampleRate;

		bool doEnvelopes =
		    (lengthMSPerSlice >= 90); // Only do fades in and out if we've got at least 100ms to play with

		firstRange->sampleHolder.startPos = 0;
		uint32_t nextDrumStart = lengthInSamples / numClips;
		firstRange->sampleHolder.endPos = nextDrumStart;

		firstDrum->sources[0].repeatMode = getBatchRepeatMode(lengthMSPerSlice);

		firstDrum->sources[0].sampleControls.reversed = false;
		firstDrum->sources[0].sampleControls.invertReversed = false;
		firstRange->sampleHolder.claimClusterReasons(firstDrum->sources[0].sampleControls.isCurrentlyReversed(),
		                                             CLUSTER_ENQUEUE);
		if (doEnvelopes) {
			ParamCollectionSummary* summary = modelStack->paramManager->getPatchedParamSetSummary();
			ModelStackWithParamId* modelStackWithParamId =
			    modelStack->addParamCollectionAndId(summary->paramCollection, summary, params::LOCAL_ENV_0_RELEASE);
			ModelStackWithAutoParam* modelStackWithAutoParam =
			    modelStackWithParamId->paramCollection->getAutoParamFromId(modelStackWithParamId);
			modelStackWithAutoParam->autoParam->setCurrentValueWithNoReversionOrRecording(
			    modelStackWithAutoParam, getParamFromUserValue(params::LOCAL_ENV_0_RELEASE, 1));
		}

		ModelStackWithTimelineCounter* noteRowModelStack = nullptr;
		if (usesExistingKit) {
			noteRowModelStack = currentSong->setupModelStackWithCurrentClip(modelStackMemory);
		}

		// Do the rest of the Drums.
		// Their rows are placed directly above the selected first drum, rather than using the generic Kit row
		// allocator.
		for (int32_t i = 1; i < numClips; i++) {

			// Make the Drum and its ParamManager
			ParamManagerForTimeline paramManager;
			error = paramManager.setupWithPatching();
			if (error != Error::NONE) {
				goto getOut;
			}

			void* drumMemory = GeneralMemoryAllocator::get().allocMaxSpeed(sizeof(SoundDrum));
			if (!drumMemory) {
ramError:
				error = Error::INSUFFICIENT_RAM;
				goto getOut;
			}

			SoundDrum* newDrum = new (drumMemory) SoundDrum();

			MultisampleRange* range = (MultisampleRange*)newDrum->sources[0].getOrCreateFirstRange();
			if (!range) {
ramError2:
				newDrum->~SoundDrum();
				delugeDealloc(drumMemory);
				goto ramError;
			}

			newDrum->drumName = deluge::generated_slice_name::makeName(generatedSliceSeriesIndex, i + 1);

			Sound::initParams(&paramManager);

			newDrum->setupAsSample(&paramManager);

			range->sampleHolder.startPos = nextDrumStart;
			nextDrumStart = (uint64_t)lengthInSamples * (i + 1) / numClips;
			range->sampleHolder.endPos = nextDrumStart;

			newDrum->sources[0].repeatMode = getBatchRepeatMode(lengthMSPerSlice);

			range->sampleHolder.filePath.set(&sample->filePath);
			range->sampleHolder.loadFile(false, false, true);

			if (doEnvelopes) {
				paramManager.getPatchedParamSet()->params[params::LOCAL_ENV_0_ATTACK].setCurrentValueBasicForSetup(
				    getParamFromUserValue(params::LOCAL_ENV_0_ATTACK, 1));
				if (i != numClips - 1) {
					paramManager.getPatchedParamSet()->params[params::LOCAL_ENV_0_RELEASE].setCurrentValueBasicForSetup(
					    getParamFromUserValue(params::LOCAL_ENV_0_RELEASE, 1));
				}
			}

			if (usesExistingKit) {
				int32_t noteRowIndex = firstDrumNoteRowIndex + i;
				NoteRow* newNoteRow = getCurrentInstrumentClip()->noteRows.insertNoteRowAtIndex(noteRowIndex);
				if (!newNoteRow) {
					newDrum->~SoundDrum();
					delugeDealloc(drumMemory);
					goto ramError;
				}

				kit->addDrum(newDrum);
				ModelStackWithNoteRow* newNoteRowModelStack = noteRowModelStack->addNoteRow(
				    getCurrentInstrumentClip()->getNoteRowId(newNoteRow, noteRowIndex), newNoteRow);
				newNoteRow->setDrum(newDrum, kit, newNoteRowModelStack, nullptr, &paramManager);
			}
			else {
				kit->addDrum(newDrum);
				currentSong->backUpParamManager(newDrum, getCurrentClip(), &paramManager, true);
			}

			if (isManualSlice) {
				manualSliceDrums[i] = newDrum;
			}
		}

		if (!usesExistingKit) {
			// Region Slice retains its established generic Kit-row assignment behavior.
			getCurrentKit()->resetDrumTempValues();
			firstDrum->noteRowAssignedTemp = 1;
		}
	}
	if (!usesExistingKit) {
		ModelStackWithTimelineCounter* modelStack = (ModelStackWithTimelineCounter*)modelStackMemory;
		getCurrentInstrumentClip()->assignDrumsToNoteRows(modelStack);
	}

	getCurrentInstrument()->beenEdited();
	manualPreviewChangedRepeatMode = false;

	// New NoteRows have probably been created, whose colours haven't been grabbed yet.
	instrumentClipView.recalculateColours();

	display->setNextTransitionDirection(-1);
	sampleBrowser.exitAndNeverDeleteDrum();
	uiNeedsRendering(&instrumentClipView);
	return true;
}
