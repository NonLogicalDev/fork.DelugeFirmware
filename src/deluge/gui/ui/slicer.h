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

#pragma once

#include "definitions_cxx.hpp"
#include "gui/ui/slicer_batch_playback_mode.h"
#include "gui/ui/ui.h"
#include "hid/button.h"

#define SLICER_MODE_REGION 0
#define SLICER_MODE_MANUAL 1
#define MAX_MANUAL_SLICES 64

class SoundDrum;

struct SliceItem {
	int32_t startPos;
	int32_t transpose;
};

class Slicer final : public UI {
public:
	Slicer() = default;

	bool opened() override;
	void focusRegained() override;
	bool canSeeViewUnderneath() override { return false; }
	void selectEncoderAction(int8_t offset) override;
	ActionResult buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) override;
	ActionResult padAction(int32_t x, int32_t y, int32_t velocity) override;

	bool renderMainPads(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
	                    uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea) override;
	bool renderSidebar(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
	                   uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth]) override;
	void graphicsRoutine() override;
	ActionResult horizontalEncoderAction(int32_t offset) override;
	ActionResult verticalEncoderAction(int32_t offset, bool inCardRoutine) override;

	void stopAnyPreviewing();
	void preview(int64_t startPoint, int64_t endPoint, int32_t transpose, int32_t on);
	void requestInitialMode(int32_t mode) { requestedInitialMode = mode; }
	int32_t getBatchPlaybackModeMenuIndex() const;
	bool confirmWithBatchPlaybackModeMenuIndex(int32_t menuIndex);

	int32_t numManualSlice;
	int32_t currentSlice;
	int32_t slicerMode;
	SliceItem manualSlicePoints[MAX_MANUAL_SLICES];

	void renderOLED(deluge::hid::display::oled_canvas::Canvas& canvas) override;

	int16_t numClips;

	// ui
	UIType getUIType() override { return UIType::SLICER; }

private:
	int32_t requestedInitialMode = SLICER_MODE_REGION;
	bool horizontalEncoderPressed = false;
	bool horizontalEncoderPressUsed = false;
	bool usesExistingKit = false;
	deluge::gui::slicer_playback::BatchMode batchPlaybackMode = deluge::gui::slicer_playback::BatchMode::AUTO;
	SampleRepeatMode repeatModeBeforeManualPreview = SampleRepeatMode::CUT;
	bool manualPreviewChangedRepeatMode = false;
	SoundDrum* manualSliceDrums[MAX_MANUAL_SLICES]{};

	// 7SEG Only
	void redraw();

	bool doSlice();
	bool confirmSlices();
	void chooseBatchPlaybackMode(deluge::gui::slicer_playback::BatchMode newMode);
	SampleRepeatMode getBatchRepeatMode(uint32_t lengthMSPerSlice) const;
	void restoreManualPreviewRepeatMode();
};

extern Slicer slicer;
