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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace deluge::gui::views::clip_progress_ruler {

constexpr int32_t kMinimumMarkSpacing = 4;
constexpr size_t kMaxMarkCount = OLED_MAIN_WIDTH_PIXELS / kMinimumMarkSpacing;
constexpr int32_t kStripHeight = 3;

struct Viewport {
	int32_t start{};
	int32_t end{};

	[[nodiscard]] constexpr bool valid() const { return start < end; }
	bool operator==(Viewport const&) const = default;
};

enum class ViewportPolicy : uint8_t { TIMELINE, NONE };

[[nodiscard]] constexpr Viewport viewportForPolicy(ViewportPolicy policy, Viewport timelineViewport) {
	return policy == ViewportPolicy::TIMELINE ? timelineViewport : Viewport{};
}

/// Use the captured extent as the provisional whole-Clip domain while a linear recording still carries a sentinel
/// loop length. The addition saturates only at the uint32 limit, beyond the firmware's supported sequence length.
[[nodiscard]] constexpr uint32_t growingLoopLength(uint32_t livePosition) {
	return livePosition == std::numeric_limits<uint32_t>::max() ? livePosition : livePosition + 1;
}

/// Map a position within one complete Clip loop to its fixed OLED column.
[[nodiscard]] constexpr int32_t loopPositionToX(uint64_t position, uint32_t clipLength) {
	if (clipLength == 0 || position >= clipLength) {
		return -1;
	}
	return static_cast<int32_t>((position * OLED_MAIN_WIDTH_PIXELS) / clipLength);
}

/// Map an exclusive boundary within one complete Clip loop to the final pixel covered before it.
[[nodiscard]] constexpr int32_t loopEndBoundaryToX(uint64_t position, uint32_t clipLength) {
	if (clipLength == 0 || position == 0 || position > clipLength) {
		return -1;
	}
	return static_cast<int32_t>((position * OLED_MAIN_WIDTH_PIXELS + clipLength - 1) / clipLength - 1);
}

struct Mark {
	uint8_t x{};
	bool isBar{};

	bool operator==(Mark const&) const = default;
};

struct StaticGeometry {
	bool visible{};
	bool selectionVisible{};
	uint8_t selectionStartX{};
	uint8_t selectionEndX{};
	std::array<Mark, kMaxMarkCount> marks{};
	uint8_t markCount{};

	bool operator==(StaticGeometry const&) const = default;
};

struct MovingGeometry {
	bool visible{};
	uint8_t playheadX{};

	bool operator==(MovingGeometry const&) const = default;
};

struct RefreshFrame {
	MovingGeometry movingGeometry{};
	uint32_t staticGeometryRevision{};
	uint32_t mainImageGeneration{};

	bool operator==(RefreshFrame const&) const = default;
};

struct RefreshState {
	RefreshFrame frame{};
	uint32_t lastRefreshTime{};
	bool valid{};
	bool transportMoving{};
	bool pinToRightEdge{};
};

struct RefreshDecision {
	bool draw{};
	bool markDirty{};
	bool checkpointCadence{};
};

enum class RepeatedDirection : uint8_t { FORWARD, REVERSE, PINGPONG };

/// Re-project a recording clone's longer live position onto the displayed source Clip, matching Session playheads.
[[nodiscard]] constexpr uint32_t mapRepeatedPosition(uint32_t livePosition, uint32_t loopLength,
                                                     RepeatedDirection direction) {
	if (loopLength == 0) {
		return 0;
	}

	const uint32_t repeat = livePosition / loopLength;
	uint32_t position = livePosition - repeat * loopLength;
	if (direction == RepeatedDirection::REVERSE || (direction == RepeatedDirection::PINGPONG && (repeat & 1U) != 0)) {
		if (position != 0) {
			position = loopLength - position;
		}
	}
	return position;
}

namespace detail {

[[nodiscard]] constexpr uint64_t minimumSparseStep(uint32_t clipLength) {
	return (static_cast<uint64_t>(clipLength) * kMinimumMarkSpacing + OLED_MAIN_WIDTH_PIXELS - 1)
	       / OLED_MAIN_WIDTH_PIXELS;
}

constexpr void populateMarks(StaticGeometry& geometry, uint32_t clipLength, uint64_t step, uint32_t barLength,
                             bool barsOnly) {
	for (uint64_t position = 0; step != 0 && position < clipLength && geometry.markCount < geometry.marks.size();
	     position += step) {
		const int32_t x = loopPositionToX(position, clipLength);
		if (x < 0) {
			continue;
		}
		geometry.marks[geometry.markCount++] = {
		    .x = static_cast<uint8_t>(x),
		    .isBar = barsOnly || (barLength != 0 && position % barLength == 0),
		};
	}
}

} // namespace detail

[[nodiscard]] constexpr StaticGeometry calculateStaticGeometry(Viewport const& viewport, uint32_t clipLength,
                                                               uint32_t quarterNoteLength, uint32_t barLength) {
	StaticGeometry geometry{};
	if (clipLength == 0) {
		return geometry;
	}
	geometry.visible = true;

	if (viewport.valid()) {
		const int64_t selectionStart = std::max<int64_t>(0, viewport.start);
		const int64_t selectionEnd = std::min<int64_t>(clipLength, viewport.end);
		if (selectionStart < selectionEnd) {
			geometry.selectionVisible = true;
			geometry.selectionStartX =
			    static_cast<uint8_t>(loopPositionToX(static_cast<uint64_t>(selectionStart), clipLength));
			geometry.selectionEndX =
			    static_cast<uint8_t>(loopEndBoundaryToX(static_cast<uint64_t>(selectionEnd), clipLength));
		}
	}

	const uint64_t minimumMarkStep = detail::minimumSparseStep(clipLength);
	if (quarterNoteLength != 0 && quarterNoteLength >= minimumMarkStep) {
		detail::populateMarks(geometry, clipLength, quarterNoteLength, barLength, false);
		return geometry;
	}

	uint64_t stride = barLength;
	while (stride != 0 && stride < minimumMarkStep) {
		stride *= 2;
	}
	if (stride != 0) {
		detail::populateMarks(geometry, clipLength, stride, barLength, true);
	}
	return geometry;
}

[[nodiscard]] constexpr MovingGeometry calculateMovingGeometry(StaticGeometry const& staticGeometry,
                                                               uint32_t clipLength, bool moving, uint32_t playPosition,
                                                               bool pinToRightEdge = false) {
	MovingGeometry geometry{};
	if (!moving || !staticGeometry.visible || clipLength == 0) {
		return geometry;
	}

	geometry.visible = true;
	geometry.playheadX = pinToRightEdge ? OLED_MAIN_WIDTH_PIXELS - 1
	                                    : static_cast<uint8_t>(loopPositionToX(playPosition % clipLength, clipLength));
	return geometry;
}

/// Unsigned subtraction keeps the cadence correct when the 32-bit audio sample timer wraps.
[[nodiscard]] constexpr bool refreshDue(uint32_t now, uint32_t previous, uint32_t interval, bool urgent) {
	return urgent || static_cast<uint32_t>(now - previous) >= interval;
}

/// A full OLED redraw still needs the cached ruler painted onto the replacement canvas, but it must not sample a new
/// moving frame before the shared cadence checkpoint.
[[nodiscard]] constexpr bool fullRenderProjectionDue(RefreshState const& displayed, uint32_t now, uint32_t interval,
                                                     bool urgent) {
	return !displayed.valid || refreshDue(now, displayed.lastRefreshTime, interval, urgent);
}

/// Decide whether a projected ruler frame must replace the cached frame and whether that hidden-canvas update should
/// request an OLED transfer. Keeping this independent of ClipView makes the actual cache and dirty protocol testable.
[[nodiscard]] constexpr RefreshDecision decideRefresh(RefreshState const& displayed, RefreshFrame const& current,
                                                      uint32_t now, uint32_t interval, bool urgent,
                                                      bool topStripCovered) {
	if (displayed.valid && displayed.frame == current) {
		return {.checkpointCadence = refreshDue(now, displayed.lastRefreshTime, interval, urgent)};
	}

	const bool mainImageReplaced =
	    displayed.valid && displayed.frame.mainImageGeneration != current.mainImageGeneration;
	if (!refreshDue(now, displayed.lastRefreshTime, interval, urgent || mainImageReplaced || !displayed.valid)) {
		return {};
	}

	return {.draw = true, .markDirty = !topStripCovered, .checkpointCadence = true};
}

template <typename Canvas>
void render(Canvas& canvas, StaticGeometry const& staticGeometry, MovingGeometry const& movingGeometry,
            int32_t top = OLED_MAIN_TOPMOST_PIXEL) {
	if (!staticGeometry.visible) {
		return;
	}

	const int32_t middle = top + 1;
	const int32_t bottom = top + kStripHeight - 1;
	if (movingGeometry.visible) {
		canvas.drawHorizontalLine(top, 0, movingGeometry.playheadX);
	}
	if (staticGeometry.selectionVisible) {
		canvas.drawHorizontalLine(middle, staticGeometry.selectionStartX, staticGeometry.selectionEndX);
	}
	for (size_t i = 0; i < staticGeometry.markCount; i++) {
		const Mark& mark = staticGeometry.marks[i];
		if (mark.isBar) {
			const int32_t start = std::min<int32_t>(mark.x, OLED_MAIN_WIDTH_PIXELS - 2);
			canvas.drawHorizontalLine(bottom, start, start + 1);
		}
		else {
			canvas.drawPixel(mark.x, bottom);
		}
	}

	canvas.drawVerticalLine(0, top, bottom);
	canvas.drawVerticalLine(OLED_MAIN_WIDTH_PIXELS - 1, top, bottom);
	if (movingGeometry.visible) {
		canvas.drawVerticalLine(movingGeometry.playheadX, top, bottom);
	}
}

} // namespace deluge::gui::views::clip_progress_ruler
