/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * The Deluge Firmware is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the Deluge Firmware.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/menu_item/panic.h"

#include "extern.h"
#include "gui/l10n/l10n.h"
#include "hid/display/display.h"
#include "processing/engines/audio_engine.h"

namespace deluge::gui::menu_item::panic {

MenuItem* Panic::selectButtonPress() {
	AudioEngine::panic();
	display->displayPopup(l10n::get(l10n::String::STRING_FOR_STOPPED));
	return NO_NAVIGATION;
}

} // namespace deluge::gui::menu_item::panic
