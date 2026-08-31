/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "definitions_cxx.hpp"
#include "gui/l10n/strings.h"

namespace deluge::gui::menu_item::voice::polyphony_policy {

enum class DetailMenu : uint8_t {
	NONE,
	VOICE_COUNT,
	CHOKE_GROUP,
};

[[nodiscard]] constexpr DetailMenu detailMenuFor(PolyphonyMode mode, bool editingKitRow) {
	if (mode == PolyphonyMode::POLY) {
		return DetailMenu::VOICE_COUNT;
	}
	if (mode == PolyphonyMode::CHOKE && editingKitRow) {
		return DetailMenu::CHOKE_GROUP;
	}
	return DetailMenu::NONE;
}

[[nodiscard]] constexpr l10n::String chokeGroupTitle(bool haveOLED) {
	return haveOLED ? l10n::String::STRING_FOR_CHOKE_GROUP : l10n::String::STRING_FOR_CHOKE_GROUP_SHORT;
}

} // namespace deluge::gui::menu_item::voice::polyphony_policy
