/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "gui/context_menu/slicer_playback_mode.h"
#include "gui/l10n/l10n.h"
#include "gui/ui/slicer.h"
#include "gui/ui/slicer_batch_playback_mode.h"
#include "gui/ui/ui.h"
#include "hid/display/display.h"

namespace deluge::gui::context_menu {

SlicerPlaybackMode slicerPlaybackMode{};

bool SlicerPlaybackMode::setupAndCheckAvailability() {
	currentOption = slicer.getBatchPlaybackModeMenuIndex();
	scrollPos = currentOption;
	return true;
}

char const* SlicerPlaybackMode::getTitle() {
	return l10n::get(l10n::String::STRING_FOR_SLICE);
}

std::span<char const*> SlicerPlaybackMode::getOptions() {
	using enum l10n::String;
	static char const* options[] = {
	    l10n::get(STRING_FOR_AUTO),
	    l10n::get(STRING_FOR_CUT),
	    l10n::get(STRING_FOR_ONCE),
	};
	return options;
}

bool SlicerPlaybackMode::acceptCurrentOption() {
	if (currentOption < slicer_playback::toMenuIndex(slicer_playback::BatchMode::AUTO)
	    || currentOption > slicer_playback::toMenuIndex(slicer_playback::BatchMode::ONCE)) {
		return true;
	}

	const int32_t acceptedOption = currentOption;
	display->setNextTransitionDirection(-1);
	close();
	slicer.confirmWithBatchPlaybackModeMenuIndex(acceptedOption);

	// The menu is already closed so Slicer can safely close its own UI stack on successful confirmation.
	return true;
}

ActionResult SlicerPlaybackMode::padAction(int32_t x, int32_t y, int32_t on) {
	if (!on) {
		return slicer.padAction(x, y, on);
	}
	return ContextMenu::padAction(x, y, on);
}

ActionResult SlicerPlaybackMode::buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) {
	if (b == deluge::hid::button::X_ENC && !on) {
		return slicer.buttonAction(b, on, inCardRoutine);
	}
	return ContextMenu::buttonAction(b, on, inCardRoutine);
}

} // namespace deluge::gui::context_menu
