/*
 * Copyright © 2019-2023 Synthstrom Audible Limited
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

#include "gui/views/audio_clip_view.h"
#include "definitions_cxx.hpp"
#include "deluge/model/settings/runtime_feature_settings.h"
#include "extern.h"
#include "gui/colour/colour.h"
#include "gui/l10n/l10n.h"
#include "gui/ui/sound_editor.h"
#include "gui/ui/ui.h"
#include "gui/ui_timer_manager.h"
#include "gui/views/arranger_view.h"
#include "gui/views/automation_view.h"
#include "gui/views/session_view.h"
#include "gui/views/timeline_view_navigation.h"
#include "gui/views/view.h"
#include "gui/waveform/waveform_renderer.h"
#include "hid/buttons.h"
#include "hid/display/display.h"
#include "hid/led/indicator_leds.h"
#include "hid/led/pad_leds.h"
#include "hid/matrix/matrix_driver.h"
#include "model/action/action_logger.h"
#include "model/clip/audio_clip.h"
#include "model/clip/audio_clip_bound_edit.h"
#include "model/clip/clip_minder.h"
#include "model/consequence/consequence_clip_length.h"
#include "model/model_stack.h"
#include "model/sample/sample.h"
#include "model/sample/sample_playback_guide.h"
#include "model/sample/sample_recorder.h"
#include "model/song/song.h"
#include "playback/mode/arrangement.h"
#include "playback/mode/playback_mode.h"
#include "playback/mode/session.h"
#include "playback/playback_handler.h"
#include "processing/engines/audio_engine.h"
#include "storage/flash_storage.h"
#include <algorithm>
#include <limits>

extern "C" {
extern uint8_t currentlyAccessingCard;
}

using namespace deluge::gui;

PLACE_SDRAM_BSS AudioClipView audioClipView{};

inline Sample* getSample() {
	AudioClip& clip = *getCurrentAudioClip();
	if (clip.getCurrentlyRecordingLinearly()) {
		return clip.recorder->sample;
	}
	return static_cast<Sample*>(clip.sampleHolder.audioFile);
}

bool AudioClipView::opened() {
	mustRedrawTickSquares = true;
	uiNeedsRendering(this);

	getCurrentClip()->onAutomationClipView = false;

	focusRegained();
	return true;
}

void AudioClipView::focusRegained() {
	closeMarkerGesture();
	ClipView::focusRegained();
	endMarkerVisible = false;
	startMarkerVisible = false;
	markerGestureActive = false;
	markerGestureTickOffset = 0;
	indicator_leds::setLedState(IndicatorLED::BACK, false);
	view.focusRegained();
	view.setActiveModControllableTimelineCounter(getCurrentClip());

	if (display->have7SEG()) {
		view.displayOutputName(getCurrentOutput(), false);
	}
#ifdef currentClipStatusButtonX
	view.drawCurrentClipPad(getCurrentClip());
#endif
}

void AudioClipView::closeMarkerGesture() {
	if (markerGestureActive) {
		actionLogger.closeAction(ActionType::AUDIO_CLIP_MARKER_EDIT);
	}
	markerGestureActive = false;
	markerGestureTickOffset = 0;
}

void AudioClipView::clearMarkerSelection() {
	closeMarkerGesture();
	endMarkerVisible = false;
	startMarkerVisible = false;
	if (getCurrentUI() == this) {
		uiTimerManager.unsetTimer(TimerName::UI_SPECIFIC);
	}
}

void AudioClipView::renderOLED(deluge::hid::display::oled_canvas::Canvas& canvas) {
	view.displayOutputName(getCurrentOutput(), false, getCurrentClip());
	renderClipProgressRuler(canvas, ClipProgressRulerKind::AUDIO);
}

bool AudioClipView::renderMainPads(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
                                   uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea) {
	if (!image) {
		return true;
	}

	if (isUIModeActive(UI_MODE_INSTRUMENT_CLIP_COLLAPSING) || isUIModeActive(UI_MODE_IMPLODE_ANIMATION)) {
		return true;
	}
	if (maybeRenderShortcutsOverview(whichRows, image, occupancyMask, drawUndefinedArea)) {
		return true;
	}

	// If no Sample, just clear display
	if (!getSample()) {
		for (int32_t y = 0; y < kDisplayHeight; y++) {
			memset(image[y], 0, kDisplayWidth * 3);
		}
		return true;
	}

	// If no audio clip, clear display
	AudioClip* clipPtr = getCurrentAudioClip();
	if (!clipPtr) {
		for (int32_t y = 0; y < kDisplayHeight; y++) {
			memset(image[y], 0, kDisplayWidth * 3);
		}
		return true;
	}

	AudioClip& clip = *clipPtr;
	SampleRecorder* recorder = clip.recorder;
	const bool startEditingEnabled = runtimeFeatureSettings.get(RuntimeFeatureSettingType::TrimFromStartOfAudioClip);

	// end marker column
	int32_t endSquareDisplay = divide_round_negative(clip.loopLength - currentSong->xScroll[NAVIGATION_CLIP] - 1,
	                                                 currentSong->xZoom[NAVIGATION_CLIP]);

	// start marker column
	int32_t startSquareDisplay =
	    divide_round_negative(0 - currentSong->xScroll[NAVIGATION_CLIP], currentSong->xZoom[NAVIGATION_CLIP]);

	int64_t xScrollSamples;
	int64_t xZoomSamples;
	clip.getScrollAndZoomInSamples(currentSong->xScroll[NAVIGATION_CLIP], currentSong->xZoom[NAVIGATION_CLIP],
	                               &xScrollSamples, &xZoomSamples);

	RGB rgb = clip.getColour();

	// Adjust xEnd if end marker is blinking
	int32_t visibleWaveformXEnd = endSquareDisplay + 1;
	if (endMarkerVisible && blinkOn) {
		visibleWaveformXEnd--;
	}
	int32_t xEnd = std::min(kDisplayWidth, visibleWaveformXEnd);

	bool success = waveformRenderer.renderFullScreen(getSample(), xScrollSamples, xZoomSamples, image, &clip.renderData,
	                                                 recorder, rgb, clip.sampleControls.isCurrentlyReversed(), xEnd);

	// If card being accessed and waveform would have to be re-examined, come back later
	if (!success && image == PadLEDs::image) {
		uiNeedsRendering(this, whichRows, 0);
		return true;
	}

	// If asked, draw grey regions + flashing columns
	if (drawUndefinedArea) {
		for (int32_t y = 0; y < kDisplayHeight; y++) {

			// -------- END marker ----------
			if (endSquareDisplay < kDisplayWidth) {
				if (endSquareDisplay >= 0) {
					image[y][endSquareDisplay] = endMarkerVisible && blinkOn ? colours::red : colours::red_dull;
				}
				int32_t xDisplay = endSquareDisplay + 1;
				if (xDisplay < kDisplayWidth) {
					if (xDisplay < 0) {
						xDisplay = 0;
					}
					RGB greyCol = colours::grey;
					std::fill(&image[y][xDisplay], &image[y][kDisplayWidth], greyCol);
				}
			}

			// -------- START marker ----------

			if (startEditingEnabled && startSquareDisplay > 0) {
				const int32_t preStartEnd = std::min(startSquareDisplay, kDisplayWidth);
				for (int32_t xDisplay = 0; xDisplay < preStartEnd; xDisplay++) {
					image[y][xDisplay] = image[y][xDisplay].dim(2);
				}
			}
			if (startEditingEnabled && startSquareDisplay >= 0 && startSquareDisplay < kDisplayWidth) {
				image[y][startSquareDisplay] = startMarkerVisible && blinkOn ? colours::green : colours::green.dim(2);
			}
			if (startEditingEnabled && startSquareDisplay == endSquareDisplay && startSquareDisplay >= 0
			    && startSquareDisplay < kDisplayWidth) {
				if (startMarkerVisible) {
					image[y][startSquareDisplay] = blinkOn ? colours::green : colours::green.dim(2);
				}
				else if (endMarkerVisible) {
					image[y][startSquareDisplay] = blinkOn ? colours::red : colours::red_dull;
				}
				else {
					image[y][startSquareDisplay] = colours::yellow.dim(2);
				}
			}
		}
	}

	return true;
}

ActionResult AudioClipView::timerCallback() {
	if (!startMarkerVisible && !endMarkerVisible) {
		return ActionResult::DEALT_WITH;
	}
	blinkOn = !blinkOn;
	uiNeedsRendering(this, 0xFFFFFFFF, 0); // Very inefficient!

	uiTimerManager.setTimer(TimerName::UI_SPECIFIC, kSampleMarkerBlinkTime);
	return ActionResult::DEALT_WITH;
}

bool AudioClipView::renderSidebar(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
                                  uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth]) {
	if (!image) {
		return true;
	}

	if (isUIModeActive(UI_MODE_INSTRUMENT_CLIP_COLLAPSING) || isUIModeActive(UI_MODE_IMPLODE_ANIMATION)) {
		return true;
	}

	int32_t macroColumn = kDisplayWidth;
	bool armed = false;
	for (int32_t y = 0; y < kDisplayHeight; y++) {
		RGB* const start = &image[y][kDisplayWidth];
		std::fill(start, start + kSideBarWidth, colours::black);

		if (isUIModeActive(UI_MODE_HOLDING_SONG_BUTTON)) {
			armed |= view.renderMacros(macroColumn, y, -1, image, occupancyMask);
		}

		if (occupancyMask) {
			PadLEDs::refreshSidebarOccupancy(image[y], occupancyMask[y]);
		}
	}
	if (armed) {
		view.flashPlayEnable();
	}

	return true;
}

void AudioClipView::graphicsRoutine() {
	refreshClipProgressRuler(ClipProgressRulerKind::AUDIO);
	if (isUIModeActive(UI_MODE_AUDIO_CLIP_COLLAPSING)) {
		return;
	}

	int32_t newTickSquare;

	if (!playbackHandler.playbackState || !currentSong->isClipActive(getCurrentClip())
	    || currentUIMode == UI_MODE_EXPLODE_ANIMATION || currentUIMode == UI_MODE_IMPLODE_ANIMATION
	    || playbackHandler.ticksLeftInCountIn) {
		newTickSquare = 255;
	}
	// Tempoless or arranger recording
	else if (!playbackHandler.isEitherClockActive()
	         || (currentPlaybackMode == &arrangement && getCurrentClip()->getCurrentlyRecordingLinearly())) {
		newTickSquare = kDisplayWidth - 1;

		// Linearly recording
		if (getCurrentClip()->getCurrentlyRecordingLinearly()) { // This would have to be true if we got here, I think?
			getCurrentAudioClip()->renderData.xScroll = -1;      // Make sure values are recalculated
			needsRenderingDependingOnSubMode();
		}
	}
	else {
		newTickSquare = getTickSquare();

		if (getCurrentAudioClip()->getCurrentlyRecordingLinearly()) {
			needsRenderingDependingOnSubMode();
		}
		if (newTickSquare < 0 || newTickSquare >= kDisplayWidth) {
			newTickSquare = 255;
		}
	}

	if (PadLEDs::flashCursor != FLASH_CURSOR_OFF && (newTickSquare != lastTickSquare || mustRedrawTickSquares)) {
		uint8_t tickSquares[kDisplayHeight];
		memset(tickSquares, newTickSquare, kDisplayHeight);

		std::array<uint8_t, 8> coloursArray = {0};
		if (getCurrentClip()->getCurrentlyRecordingLinearly()) {
			coloursArray.fill(2);
		}

		PadLEDs::setTickSquares(tickSquares, coloursArray.data());

		lastTickSquare = newTickSquare;
		mustRedrawTickSquares = false;
	}
}

void AudioClipView::needsRenderingDependingOnSubMode() {
	switch (currentUIMode) {
	case UI_MODE_HORIZONTAL_SCROLL:
	case UI_MODE_HORIZONTAL_ZOOM:
		break;
	default:
		uiNeedsRendering(this, 0xFFFFFFFF, 0);
	}
}

// If you want your specialized button logic (session view, clip view, etc.),
// put that here. Otherwise call the parent:
ActionResult AudioClipView::buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) {
	using namespace deluge::hid::button;

	maybeStartShortcutOverview(b, on);
	ActionResult result;
	if (on && (startMarkerVisible || endMarkerVisible)) {
		if (b == X_ENC || b == SHIFT) {
			closeMarkerGesture();
		}
		else {
			clearMarkerSelection();
			uiNeedsRendering(this, 0xFFFFFFFF, 0);
		}
	}

	// Song view button
	if (b == SESSION_VIEW) {
		if (on) {
			if (currentUIMode == UI_MODE_NONE) {
				currentUIMode = UI_MODE_HOLDING_SONG_BUTTON;
				timeSongButtonPressed = AudioEngine::audioSampleTimer;
				indicator_leds::setLedState(IndicatorLED::SESSION_VIEW, true);
				uiNeedsRendering(this, 0, 0xFFFFFFFF);
			}
		}
		else {
			if (!isUIModeActive(UI_MODE_HOLDING_SONG_BUTTON)) {
				return ActionResult::DEALT_WITH;
			}
			if (inCardRoutine) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}
			exitUIMode(UI_MODE_HOLDING_SONG_BUTTON);
			if ((int32_t)(AudioEngine::audioSampleTimer - timeSongButtonPressed) > kShortPressTime) {
				uiNeedsRendering(this, 0, 0xFFFFFFFF);
				indicator_leds::setLedState(IndicatorLED::SESSION_VIEW, false);
				return ActionResult::DEALT_WITH;
			}

			uiTimerManager.unsetTimer(TimerName::UI_SPECIFIC);

			ClipMinder::transitionToArrangerOrSession();
		}
	}

	// Clip view button
	else if (b == CLIP_VIEW) {
		if (on && currentUIMode == UI_MODE_NONE) {
			if (inCardRoutine) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}

			changeRootUI(&automationView);
		}
	}

	else if (b == PLAY) {
dontDeactivateMarker:
		return ClipView::buttonAction(b, on, inCardRoutine);
	}

	else if (b == RECORD) {
		goto dontDeactivateMarker;
	}

	else if (b == SHIFT) {
		goto dontDeactivateMarker;
	}

	else if (b == X_ENC) {
		// removing time stretching by re-calculating clip length based on length of audio sample
		if (Buttons::isButtonPressed(deluge::hid::button::Y_ENC)) {
			if (on && currentUIMode == UI_MODE_NONE) {
				if (inCardRoutine) {
					return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
				}
				clearMarkerSelection();
				setClipLengthEqualToSampleLength();
			}
		}
		// if shift is pressed then we're resizing the clip without time stretching
		else if (!Buttons::isShiftButtonPressed()) {
			goto dontDeactivateMarker;
		}
	}

	// Select button, without shift
	else if (b == SELECT_ENC && !Buttons::isShiftButtonPressed()) {
		if (on && currentUIMode == UI_MODE_NONE) {
			if (inCardRoutine) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}

			if (!soundEditor.setup(getCurrentClip())) {
				return ActionResult::DEALT_WITH;
			}
			openUI(&soundEditor);

			result = ActionResult::DEALT_WITH;
			goto deactivateMarkerIfNecessary;
		}
	}

	// Back button to clear Clip
	else if (b == BACK && currentUIMode == UI_MODE_HOLDING_HORIZONTAL_ENCODER_BUTTON) {
		if (on) {
			if (inCardRoutine) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}

			// Clear Clip
			Action* action = actionLogger.getNewAction(ActionType::CLIP_CLEAR, ActionAddition::NOT_ALLOWED);

			char modelStackMemory[MODEL_STACK_MAX_SIZE];
			ModelStackWithTimelineCounter* modelStack =
			    setupModelStackWithTimelineCounter(modelStackMemory, currentSong, getCurrentClip());

			getCurrentAudioClip()->clear(action, modelStack, !FlashStorage::automationClear, true);

			// New default as part of Automation Clip View Implementation
			// If this is enabled, then when you are in Audio Clip View, clearing
			// a clip will only clear the Audio Sample (automations remain intact). If this is enabled, if you want to
			// clear automations, you will enter Automation Clip View and clear the clip there. If this is enabled, the
			// message displayed on the OLED screen is adjusted to reflect the nature of what is being cleared
			if (FlashStorage::automationClear) {
				display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_SAMPLE_CLEARED));
			}
			else {
				display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_CLIP_CLEARED));
			}
			clearMarkerSelection();
			uiNeedsRendering(this, 0xFFFFFFFF, 0);
		}
	}
	else {

		result = ClipMinder::buttonAction(b, on);
		if (result == ActionResult::NOT_DEALT_WITH) {
			result = ClipView::buttonAction(b, on, inCardRoutine);
		}

		if (result != ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE) {
deactivateMarkerIfNecessary:
			if (endMarkerVisible || startMarkerVisible) {
				clearMarkerSelection();
				uiNeedsRendering(this, 0xFFFFFFFF, 0);
			}
		}

		return result;
	}

	return ActionResult::DEALT_WITH;
}

ActionResult AudioClipView::padAction(int32_t x, int32_t y, int32_t on) {
	if (x < kDisplayWidth) {
		if (Buttons::isButtonPressed(deluge::hid::button::TEMPO_ENC)) {
			if (on) {
				playbackHandler.grabTempoFromClip(getCurrentAudioClip());
			}
		}
		else {
			if (sdRoutineLock) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}

			// Maybe go to SoundEditor
			ActionResult soundEditorResult = soundEditor.potentialShortcutPadAction(x, y, on);
			if (soundEditorResult != ActionResult::NOT_DEALT_WITH) {
				if (soundEditorResult == ActionResult::DEALT_WITH) {
					clearMarkerSelection();
					uiNeedsRendering(this, 0xFFFFFFFF, 0);
				}
				return soundEditorResult;
			}
			else if (on && !currentUIMode) {
				AudioClip* clip = getCurrentAudioClip();
				if (!clip) {
					return ActionResult::DEALT_WITH;
				}
				AudioClip& clipRef = *clip;

				int32_t endSquareDisplay =
				    divide_round_negative(clipRef.loopLength - currentSong->xScroll[NAVIGATION_CLIP] - 1,
				                          currentSong->xZoom[NAVIGATION_CLIP]);

				int32_t startSquareDisplay = divide_round_negative(0 - currentSong->xScroll[NAVIGATION_CLIP],
				                                                   currentSong->xZoom[NAVIGATION_CLIP]);
				const bool startEditingEnabled =
				    runtimeFeatureSettings.get(RuntimeFeatureSettingType::TrimFromStartOfAudioClip);
				const bool markersCoincide = startEditingEnabled && startSquareDisplay == endSquareDisplay;

				auto selectMarker = [this](bool selectStart) {
					const bool alreadySelected = selectStart ? startMarkerVisible : endMarkerVisible;
					clearMarkerSelection();
					if (!alreadySelected) {
						startMarkerVisible = selectStart;
						endMarkerVisible = !selectStart;
						blinkOn = true;
						uiTimerManager.setTimer(TimerName::UI_SPECIFIC, kSampleMarkerBlinkTime);
					}
					uiNeedsRendering(this, 0xFFFFFFFF, 0);
				};

				if (markersCoincide && x == startSquareDisplay) {
					selectMarker(startMarkerVisible ? false : true);
				}
				else if (markersCoincide && x == endSquareDisplay + 1 && x < kDisplayWidth) {
					selectMarker(false);
				}
				else if (x == endSquareDisplay || (x == endSquareDisplay + 1 && !endMarkerVisible)) {
					selectMarker(false);
				}
				else if (x == startSquareDisplay) {
					if (startEditingEnabled) {
						selectMarker(true);
					}
					else if (endMarkerVisible) {
						clearMarkerSelection();
						uiNeedsRendering(this, 0xFFFFFFFF, 0);
					}
				}
				else if (endMarkerVisible) {
					closeMarkerGesture();
					if (beginMarkerGesture()) {
						const int64_t targetLength = static_cast<int64_t>(x + 1) * currentSong->xZoom[NAVIGATION_CLIP]
						                             + currentSong->xScroll[NAVIGATION_CLIP];
						const int64_t desiredTickOffset = targetLength - markerGestureLoopLength;
						if (desiredTickOffset != 0) {
							const int8_t direction = desiredTickOffset < 0 ? -1 : 1;
							markerGestureTickOffset =
							    desiredTickOffset
							    - static_cast<int64_t>(direction) * currentSong->xZoom[NAVIGATION_CLIP];
							moveSelectedMarker(direction);
						}
						closeMarkerGesture();
					}
				}
				else if (startMarkerVisible) {
					clearMarkerSelection();
					uiNeedsRendering(this, 0xFFFFFFFF, 0);
				}
			}
		}
	}
	else if (x == kDisplayWidth) {
		if (isUIModeActive(UI_MODE_HOLDING_SONG_BUTTON)) {
			if (sdRoutineLock) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}
			// render that you're holding the macro
			if (on) {
				uiNeedsRendering(this, 0, 0xFFFFFFFF);
			}
			// activate macro on release
			else {
				view.activateMacro(y);
			}
			return ActionResult::DEALT_WITH;
		}
	}
	return ActionResult::DEALT_WITH;
}

// ----------- "End" pointer logic -----------
void AudioClipView::changeUnderlyingSampleLength(AudioClip& clip, const Sample* sample, int32_t newLength,
                                                 int32_t oldLength, uint64_t oldLengthSamples) const {
	uint64_t* valueToChange;
	int64_t newEndPosSamples;

	uint64_t newLengthSamples =
	    (uint64_t)(oldLengthSamples * (uint64_t)newLength + (oldLength >> 1)) / (uint32_t)oldLength;

	// If end pos less than 0, not allowed
	if (clip.sampleControls.isCurrentlyReversed()) {
		newEndPosSamples = clip.sampleHolder.endPos - newLengthSamples;
		if (newEndPosSamples < 0) {
			newEndPosSamples = 0;
		}
		valueToChange = &clip.sampleHolder.startPos;
	}
	// AudioClip playing forward
	else {
		newEndPosSamples = clip.sampleHolder.startPos + newLengthSamples;
		if (newEndPosSamples > sample->lengthInSamples) {
			newEndPosSamples = sample->lengthInSamples;
		}
		valueToChange = &clip.sampleHolder.endPos;

		// If the end pos is very close to the end pos marked in the audio file...
		if (sample->fileLoopStartSamples) {
			int64_t distanceFromFileEndMarker = newEndPosSamples - (uint64_t)sample->fileLoopStartSamples;
			if (distanceFromFileEndMarker < 0) {
				distanceFromFileEndMarker = -distanceFromFileEndMarker;
			}
			if (distanceFromFileEndMarker < 10) {
				newEndPosSamples = sample->fileLoopStartSamples;
			}
		}
	}

	ActionType actionType =
	    (newLength < oldLength) ? ActionType::CLIP_LENGTH_DECREASE : ActionType::CLIP_LENGTH_INCREASE;

	// Change sample end-pos value. Must do this before calling setClipLength(), which will end
	// up reading this value.
	uint64_t oldValue = *valueToChange;
	*valueToChange = newEndPosSamples;

	Action* action = actionLogger.getNewAction(actionType, ActionAddition::NOT_ALLOWED);
	currentSong->setClipLength(&clip, newLength, action);
	if (action) {
		if (action->firstConsequence && action->firstConsequence->type == Consequence::CLIP_LENGTH) {
			ConsequenceClipLength* consequence = (ConsequenceClipLength*)action->firstConsequence;
			consequence->pointerToMarkerValue = valueToChange;
			consequence->markerValueToRevertTo = oldValue;
		}
		actionLogger.closeAction(actionType);
	}
}

void AudioClipView::playbackEnded() {
	uiNeedsRendering(this, 0xFFFFFFFF, 0);
}

void AudioClipView::clipNeedsReRendering(Clip* c) {
	if (c == getCurrentAudioClip()) {
		// Scroll back left if we need to - it's possible that the length just reverted, if recording got aborted.
		// Ok, coming back to this, it seems it was a bit hacky that I put this in this function...
		if (currentSong->xScroll[NAVIGATION_CLIP] >= c->loopLength) {
			horizontalScrollForLinearRecording(0);
		}
		else {
			uiNeedsRendering(this, 0xFFFFFFFF, 0);
		}
	}
}

void AudioClipView::sampleNeedsReRendering(Sample* s) {
	if (s == getSample()) {
		uiNeedsRendering(this, 0xFFFFFFFF, 0);
	}
}

bool AudioClipView::beginMarkerGesture() {
	AudioClip* clip = getCurrentAudioClip();
	if (!clip || clip->getCurrentlyRecordingLinearly() || !clip->sampleHolder.audioFile) {
		return false;
	}

	Sample* sample = static_cast<Sample*>(clip->sampleHolder.audioFile);
	if (sample->lengthInSamples == 0 || clip->sampleHolder.startPos >= clip->sampleHolder.endPos
	    || clip->sampleHolder.endPos > sample->lengthInSamples || clip->loopLength < 1
	    || clip->loopLength > kMaxSequenceLength) {
		return false;
	}

	markerGestureRawStart = clip->sampleHolder.startPos;
	markerGestureRawEnd = clip->sampleHolder.endPos;
	markerGestureSourceLength = sample->lengthInSamples;
	markerGestureLoopLength = clip->loopLength;
	markerGestureTickOffset = 0;
	markerGestureActive = true;
	return true;
}

void AudioClipView::moveSelectedMarker(int8_t offset) {
	if (offset == 0 || (!startMarkerVisible && !endMarkerVisible) || getCurrentUI() != this || sdRoutineLock
	    || currentlyAccessingCard || playbackHandler.ticksLeftInCountIn
	    || playbackHandler.recording != RecordingMode::OFF) {
		return;
	}

	AudioClip* clip = getCurrentAudioClip();
	if (!clip || clip->getCurrentlyRecordingLinearly() || !clip->currentlyScrollableAndZoomable()) {
		return;
	}
	if (!markerGestureActive && !beginMarkerGesture()) {
		return;
	}

	const int64_t tickDelta = static_cast<int64_t>(offset) * currentSong->xZoom[NAVIGATION_CLIP];
	const int64_t proposedTickOffset = std::clamp<int64_t>(
	    markerGestureTickOffset + tickDelta, -static_cast<int64_t>(kMaxSequenceLength), kMaxSequenceLength);
	const auto selectedBound = startMarkerVisible ? deluge::audio_clip_bound_edit::PlaybackBound::START
	                                              : deluge::audio_clip_bound_edit::PlaybackBound::END;
	const bool reversed = clip->sampleControls.isCurrentlyReversed();
	const deluge::audio_clip_bound_edit::GestureAnchor anchor{
	    .rawStart = markerGestureRawStart,
	    .rawEnd = markerGestureRawEnd,
	    .loopLength = markerGestureLoopLength,
	    .sourceLength = markerGestureSourceLength,
	};

	deluge::audio_clip_bound_edit::RawBound rawBound =
	    (selectedBound == deluge::audio_clip_bound_edit::PlaybackBound::START) == !reversed
	        ? deluge::audio_clip_bound_edit::RawBound::START
	        : deluge::audio_clip_bound_edit::RawBound::END;
	uint64_t rawMarker = rawBound == deluge::audio_clip_bound_edit::RawBound::START ? anchor.rawStart : anchor.rawEnd;
	int32_t newLength = anchor.loopLength;

	if (proposedTickOffset != 0) {
		const uint32_t tickDistance =
		    static_cast<uint32_t>(proposedTickOffset < 0 ? -proposedTickOffset : proposedTickOffset);
		const auto direction = proposedTickOffset < 0 ? deluge::audio_clip_bound_edit::PlaybackDirection::EARLIER
		                                              : deluge::audio_clip_bound_edit::PlaybackDirection::LATER;
		const auto result = deluge::audio_clip_bound_edit::calculate(
		    anchor, selectedBound, reversed,
		    {.direction = direction,
		     .samplesFromAnchor = deluge::audio_clip_bound_edit::samplesForTickDistance(anchor, tickDistance)});
		if (!result) {
			if (result.error() != deluge::audio_clip_bound_edit::Rejection::NO_CHANGE) {
				return;
			}
		}
		else {
			rawBound = result->rawBound;
			rawMarker = result->rawMarker;
			newLength = result->loopLength;

			Sample* sample = static_cast<Sample*>(clip->sampleHolder.audioFile);
			if (selectedBound == deluge::audio_clip_bound_edit::PlaybackBound::END && !reversed
			    && rawBound == deluge::audio_clip_bound_edit::RawBound::END && sample->fileLoopStartSamples > 0) {
				const uint64_t fileEndMarker = static_cast<uint64_t>(sample->fileLoopStartSamples);
				const uint64_t distanceFromFileEndMarker =
				    rawMarker > fileEndMarker ? rawMarker - fileEndMarker : fileEndMarker - rawMarker;
				if (fileEndMarker <= anchor.sourceLength && distanceFromFileEndMarker < 10) {
					const auto snapDirection = fileEndMarker < anchor.rawEnd
					                               ? deluge::audio_clip_bound_edit::PlaybackDirection::EARLIER
					                               : deluge::audio_clip_bound_edit::PlaybackDirection::LATER;
					const uint64_t snapDistance =
					    fileEndMarker < anchor.rawEnd ? anchor.rawEnd - fileEndMarker : fileEndMarker - anchor.rawEnd;
					const auto snapped = deluge::audio_clip_bound_edit::calculate(
					    anchor, selectedBound, reversed,
					    {.direction = snapDirection, .samplesFromAnchor = snapDistance});
					if (snapped) {
						rawMarker = snapped->rawMarker;
						newLength = snapped->loopLength;
					}
					else if (snapped.error() == deluge::audio_clip_bound_edit::Rejection::NO_CHANGE) {
						rawMarker = anchor.rawEnd;
						newLength = anchor.loopLength;
					}
				}
			}
		}
	}

	uint64_t* markerValue = rawBound == deluge::audio_clip_bound_edit::RawBound::START ? &clip->sampleHolder.startPos
	                                                                                   : &clip->sampleHolder.endPos;
	if (*markerValue == rawMarker && clip->loopLength == newLength) {
		const bool atRawLimit = rawBound == deluge::audio_clip_bound_edit::RawBound::START
		                            ? rawMarker == 0 || rawMarker + 1 == anchor.rawEnd
		                            : rawMarker == anchor.rawStart + 1 || rawMarker == anchor.sourceLength;
		if (!atRawLimit && newLength != 1 && newLength != kMaxSequenceLength) {
			markerGestureTickOffset = proposedTickOffset;
		}
		return;
	}

	Action* action = actionLogger.getNewActionForAudioClipMarkerEdit(clip, markerValue);
	if (!action) {
		return;
	}

	const int32_t oldLength = clip->loopLength;
	const int32_t oldScroll = currentSong->xScroll[NAVIGATION_CLIP];
	*markerValue = rawMarker;
	clip->sampleHolder.claimClusterReasons(reversed, CLUSTER_LOAD_IMMEDIATELY_OR_ENQUEUE);
	currentSong->setClipLength(clip, newLength, action);
	const bool atRawLimit = rawBound == deluge::audio_clip_bound_edit::RawBound::START
	                            ? rawMarker == 0 || rawMarker + 1 == anchor.rawEnd
	                            : rawMarker == anchor.rawStart + 1 || rawMarker == anchor.sourceLength;
	if (atRawLimit) {
		// Saturate the gesture accumulator at the boundary actually reached. Otherwise one fast turn can leave a large
		// overshoot that must be unwound before the first reverse detent moves the marker. Raw position is the source
		// of truth here because its ratio back to Clip ticks may differ from the separately rounded Clip length.
		const uint64_t anchorRawMarker =
		    rawBound == deluge::audio_clip_bound_edit::RawBound::START ? anchor.rawStart : anchor.rawEnd;
		const uint64_t rawDistance =
		    rawMarker > anchorRawMarker ? rawMarker - anchorRawMarker : anchorRawMarker - rawMarker;
		const int64_t tickDistance = deluge::audio_clip_bound_edit::minimumTickDistanceForSamples(anchor, rawDistance);
		markerGestureTickOffset = proposedTickOffset < 0 ? -tickDistance : tickDistance;
	}
	else if (newLength == 1 || newLength == kMaxSequenceLength) {
		markerGestureTickOffset =
		    deluge::audio_clip_bound_edit::realizedTickOffset(selectedBound, anchor.loopLength, newLength);
	}
	else {
		markerGestureTickOffset = proposedTickOffset;
	}

	if (selectedBound == deluge::audio_clip_bound_edit::PlaybackBound::START) {
		currentSong->xScroll[NAVIGATION_CLIP] = deluge::gui::timeline_view_navigation::reanchorScrollAfterStartEdit(
		    oldScroll, oldLength, newLength, getMinXScroll());
	}
	actionLogger.updateAction(action);
	clearMarkerSelectionIfOffscreen();
	uiNeedsRendering(this, 0xFFFFFFFF, 0);
}

void AudioClipView::clearMarkerSelectionIfOffscreen() {
	if (!startMarkerVisible && !endMarkerVisible) {
		return;
	}

	const int32_t markerSquare =
	    startMarkerVisible
	        ? divide_round_negative(-currentSong->xScroll[NAVIGATION_CLIP], currentSong->xZoom[NAVIGATION_CLIP])
	        : divide_round_negative(getCurrentAudioClip()->loopLength - currentSong->xScroll[NAVIGATION_CLIP] - 1,
	                                currentSong->xZoom[NAVIGATION_CLIP]);
	if (markerSquare < 0 || markerSquare >= kDisplayWidth) {
		clearMarkerSelection();
	}
}

void AudioClipView::selectEncoderAction(int8_t offset) {
	if (currentUIMode) {
		return;
	}
	// allows you to assign an audio clip to a different audio track
	if (Buttons::isShiftButtonPressed()) {
		clearMarkerSelection();
		view.navigateThroughAudioOutputsForAudioClip(offset, getCurrentAudioClip());
	}
	else if (startMarkerVisible || endMarkerVisible) {
		moveSelectedMarker(offset);
	}
	else {
		auto ao = (AudioOutput*)getCurrentAudioClip()->output;
		ao->scrollAudioOutputMode(offset);
	}
}

void AudioClipView::setClipLengthEqualToSampleLength() {
	AudioClip& audioClip = *getCurrentAudioClip();
	SamplePlaybackGuide guide = audioClip.guide;
	SampleHolder* sampleHolder = (SampleHolder*)guide.audioFileHolder;
	if (sampleHolder) {
		adjustLoopLength(sampleHolder->getLoopLengthAtSystemSampleRate(true));
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_CLIP_LENGTH_ADJUSTED));
	}
	else {
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_NO_SAMPLE));
	}
}

void AudioClipView::adjustLoopLength(int32_t newLength) {
	int32_t oldLength = getCurrentClip()->loopLength;

	if (oldLength != newLength) {
		Action* action = nullptr;

		if (newLength > oldLength) {
			// If we're still within limits
			if (newLength <= (uint32_t)kMaxSequenceLength) {
				action = lengthenClip(newLength);
doReRender:
				// use getRootUI() in case this is called from audio clip automation view
				uiNeedsRendering(getRootUI(), 0xFFFFFFFF, 0);
			}
		}
		else if (newLength < oldLength) {
			if (newLength > 0) {
				action = shortenClip(newLength);
				// Scroll / zoom as needed
				if (!scrollLeftIfTooFarRight(newLength)) {
					if (!zoomToMax(true)) {
						goto doReRender;
					}
				}
			}
		}

		displayNumberOfBarsAndBeats(newLength, currentSong->xZoom[NAVIGATION_CLIP], false, "LONG");
		if (action) {
			action->xScrollClip[AFTER] = currentSong->xScroll[NAVIGATION_CLIP];
		}
	}
}

ActionResult AudioClipView::horizontalEncoderAction(int32_t offset) {
	stopShortcutOverview();
	closeMarkerGesture();
	// Shift and x pressed - edit length of clip without timestretching
	if (isNoUIModeActive() && Buttons::isButtonPressed(deluge::hid::button::X_ENC) && Buttons::isShiftButtonPressed()) {
		clearMarkerSelection();
		return editClipLengthWithoutTimestretching(offset);
	}
	else {
		// Otherwise, let parent do scrolling and zooming
		ActionResult result = ClipView::horizontalEncoderAction(offset);
		clearMarkerSelectionIfOffscreen();
		return result;
	}
}

ActionResult AudioClipView::editClipLengthWithoutTimestretching(int32_t offset) {
	// If tempoless recording, don't allow
	if (!getCurrentClip()->currentlyScrollableAndZoomable()) {
		display->displayPopup(deluge::l10n::get(deluge::l10n::String::STRING_FOR_CANT_EDIT_LENGTH));
		return ActionResult::DEALT_WITH;
	}

	// If we're not scrolled all the way to the right, go there now
	if (scrollRightToEndOfLengthIfNecessary(getCurrentClip()->loopLength)) {
		return ActionResult::DEALT_WITH;
	}

	if (sdRoutineLock) {
		return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
	}

	int32_t oldLength = getCurrentClip()->loopLength;
	uint64_t oldLengthSamples = getCurrentAudioClip()->sampleHolder.getDurationInSamples(true);

	Action* action = nullptr;
	uint32_t newLength = changeClipLength(offset, oldLength, action);

	AudioClip& audioClip = *getCurrentAudioClip();
	SamplePlaybackGuide guide = audioClip.guide;
	SampleHolder* sampleHolder = (SampleHolder*)guide.audioFileHolder;
	if (sampleHolder) {
		Sample* sample = static_cast<Sample*>(sampleHolder->audioFile);
		if (sample) {
			changeUnderlyingSampleLength(audioClip, sample, newLength, oldLength, oldLengthSamples);
		}
	}

	displayNumberOfBarsAndBeats(newLength, currentSong->xZoom[NAVIGATION_CLIP], false, "LONG");
	if (action) {
		action->xScrollClip[AFTER] = currentSong->xScroll[NAVIGATION_CLIP];
	}
	return ActionResult::DEALT_WITH;
}

ActionResult AudioClipView::verticalEncoderAction(int32_t offset, bool inCardRoutine) {
	stopShortcutOverview();
	if (offset != 0) {
		clearMarkerSelection();
	}
	if (!currentUIMode && Buttons::isShiftButtonPressed() && !Buttons::isButtonPressed(deluge::hid::button::Y_ENC)) {
		if (inCardRoutine && !allowSomeUserActionsEvenWhenInCardRoutine) {
			return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE; // Allow sometimes.
		}

		// Shift colour spectrum
		getCurrentAudioClip()->changeColour(offset);
		uiNeedsRendering(this, 0xFFFFFFFF, 0);
	}
	return ActionResult::DEALT_WITH;
}

bool AudioClipView::setupScroll(int32_t oldScroll) {
	if (!getCurrentAudioClip()->currentlyScrollableAndZoomable()) {
		return false;
	}
	return ClipView::setupScroll(oldScroll);
}

int32_t AudioClipView::getMinXScroll() const {
	AudioClip* clip = getCurrentAudioClip();
	if (!runtimeFeatureSettings.get(RuntimeFeatureSettingType::TrimFromStartOfAudioClip) || !clip
	    || clip->getCurrentlyRecordingLinearly() || !clip->sampleHolder.audioFile || clip->loopLength < 1) {
		return 0;
	}

	const Sample* sample = static_cast<Sample*>(clip->sampleHolder.audioFile);
	const uint64_t rawStart = clip->sampleHolder.startPos;
	const uint64_t rawEnd = std::min(clip->sampleHolder.endPos, sample->lengthInSamples);
	if (rawStart >= rawEnd) {
		return 0;
	}

	const uint64_t recoverableSamples =
	    clip->sampleControls.isCurrentlyReversed() ? sample->lengthInSamples - rawEnd : rawStart;
	return deluge::audio_clip_bound_edit::minimumSourceBackedScroll(recoverableSamples, rawEnd - rawStart,
	                                                                clip->loopLength);
}

uint32_t AudioClipView::getMaxLength() {
	if (endMarkerVisible) {
		return getCurrentClip()->loopLength + 1;
	}
	return getCurrentClip()->loopLength;
}

uint32_t AudioClipView::getMaxZoom() {
	int32_t maxZoom = getCurrentClip()->getMaxZoom();
	if (endMarkerVisible && maxZoom < 1073741824) {
		maxZoom <<= 1;
	}
	return maxZoom;
}
