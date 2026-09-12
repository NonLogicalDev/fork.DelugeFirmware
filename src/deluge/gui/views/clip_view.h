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

#pragma once

#include "gui/views/clip_navigation_timeline_view.h"
#include "gui/views/clip_progress_ruler.h"
#include "hid/button.h"

class Action;
class ModControllableAudio;
class UI;

class ClipView : public ClipNavigationTimelineView {
public:
	ClipView() = default;
	void stopShortcutOverview();

	uint32_t getMaxZoom() override;
	uint32_t getMaxLength() override;
	ActionResult horizontalEncoderAction(int32_t offset) override;
	int32_t getLengthChopAmount(int32_t square);
	int32_t getLengthExtendAmount(int32_t square);
	void focusRegained() override;
	ActionResult buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) override;

protected:
	enum class ClipProgressRulerKind : uint8_t { INSTRUMENT, AUDIO };

	int32_t getTickSquare();
	virtual ModControllableAudio* getModControllableAudioOrNone() { return nullptr; }
	void renderClipProgressRuler(deluge::hid::display::oled_canvas::Canvas& canvas, ClipProgressRulerKind kind,
	                             UI const* activeSurface = nullptr,
	                             deluge::gui::views::clip_progress_ruler::ViewportPolicy viewportPolicy =
	                                 deluge::gui::views::clip_progress_ruler::ViewportPolicy::TIMELINE);
	void refreshClipProgressRuler(ClipProgressRulerKind kind, UI const* activeSurface = nullptr,
	                              deluge::gui::views::clip_progress_ruler::ViewportPolicy viewportPolicy =
	                                  deluge::gui::views::clip_progress_ruler::ViewportPolicy::TIMELINE);

	Action* lengthenClip(int32_t newLength);
	Action* shortenClip(int32_t newLength);
	uint32_t changeClipLength(int32_t offset, uint32_t oldLength, Action*& action);

	void maybeStartShortcutOverview(deluge::hid::Button b, bool on);
	bool shouldRenderShortcutsOverview() const;
	bool maybeRenderShortcutsOverview(uint32_t whichRows, RGB image[][kDisplayWidth + kSideBarWidth],
	                                  uint8_t occupancyMask[][kDisplayWidth + kSideBarWidth], bool drawUndefinedArea);
	bool renderedShortcutOverview{false};
	bool exitedShortcutOverview{false};

private:
	struct ClipProgressRulerCache;
	struct ClipProgressRulerChanges;
	struct ClipProgressRulerInput;
	struct ClipProgressRulerUpdate;

	void collectClipProgressRulerInput(ClipProgressRulerKind kind, UI const* activeSurface,
	                                   deluge::gui::views::clip_progress_ruler::ViewportPolicy viewportPolicy,
	                                   ClipProgressRulerInput& input);
	[[nodiscard]] ClipProgressRulerChanges getClipProgressRulerChanges(ClipProgressRulerInput const& input) const;
	[[nodiscard]] ClipProgressRulerUpdate prepareClipProgressRuler(ClipProgressRulerInput const& input,
	                                                               ClipProgressRulerChanges changes);
	void recordClipProgressRulerDisplay(ClipProgressRulerUpdate const& update, uint32_t mainImageGeneration,
	                                    uint32_t now);
	void invalidateClipProgressRuler();

	static ClipProgressRulerCache clipProgressRulerCache_;
};
