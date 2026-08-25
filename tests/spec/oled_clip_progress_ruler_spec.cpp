#include "gui/views/clip_progress_ruler.h"

#include "cppspec.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace ruler = deluge::gui::views::clip_progress_ruler;

namespace {

constexpr ruler::Viewport viewport(int32_t start, int32_t end) {
	return {.start = start, .end = end};
}

class RecordingCanvas {
public:
	void drawPixel(int32_t x, int32_t y) { setPixel(x, y); }

	void drawHorizontalLine(int32_t y, int32_t startX, int32_t endX) {
		for (int32_t x = startX; x <= endX; x++) {
			setPixel(x, y);
		}
	}

	void drawVerticalLine(int32_t x, int32_t startY, int32_t endY) {
		for (int32_t y = startY; y <= endY; y++) {
			setPixel(x, y);
		}
	}

	[[nodiscard]] bool lit(int32_t x, int32_t y) const { return pixels[y][x]; }

	std::array<std::array<bool, OLED_MAIN_WIDTH_PIXELS>, OLED_MAIN_HEIGHT_PIXELS> pixels{};
	bool outOfBounds{};

private:
	void setPixel(int32_t x, int32_t y) {
		if (x < 0 || x >= OLED_MAIN_WIDTH_PIXELS || y < 0 || y >= OLED_MAIN_HEIGHT_PIXELS) {
			outOfBounds = true;
			return;
		}
		pixels[y][x] = true;
	}
};

} // namespace

static_assert(ruler::kStripHeight == 3);
static_assert(ruler::kMaxMarkCount == 32);

// clang-format off
describe oled_clip_progress_ruler("OLED Clip progress ruler", $ {
	it("maps one complete Clip loop across all 128 OLED columns", _ {
		expect(ruler::loopPositionToX(0, 1024)).to_equal(0);
		expect(ruler::loopPositionToX(511, 1024)).to_equal(63);
		expect(ruler::loopPositionToX(512, 1024)).to_equal(64);
		expect(ruler::loopPositionToX(1023, 1024)).to_equal(127);
		expect(ruler::loopPositionToX(1024, 1024)).to_equal(-1);
		expect(ruler::loopPositionToX(0, 0)).to_equal(-1);
		expect(ruler::loopEndBoundaryToX(1, 1024)).to_equal(0);
		expect(ruler::loopEndBoundaryToX(1024, 1024)).to_equal(127);
	});

	it("derives a finite growing-recording domain from the live extent", _ {
		expect(ruler::growingLoopLength(0)).to_equal(uint32_t{1});
		expect(ruler::growingLoopLength(767)).to_equal(uint32_t{768});
		expect(ruler::growingLoopLength(std::numeric_limits<uint32_t>::max()))
		    .to_equal(std::numeric_limits<uint32_t>::max());

		const uint32_t effectiveLength = ruler::growingLoopLength(767);
		const auto staticGeometry = ruler::calculateStaticGeometry(viewport(0, 256), effectiveLength, 128, 512);
		const auto movingGeometry = ruler::calculateMovingGeometry(staticGeometry, effectiveLength, true, 767, true);
		expect(staticGeometry.selectionEndX).to_equal(uint8_t{42});
		expect(staticGeometry.markCount).to_equal(uint8_t{6});
		expect(movingGeometry.playheadX).to_equal(uint8_t{127});
	});

	it("does not manufacture a static change between identical growing pixels", _ {
		const auto before = ruler::calculateStaticGeometry(viewport(0, 256), 1000, 128, 512);
		const auto after = ruler::calculateStaticGeometry(viewport(0, 256), 1001, 128, 512);
		expect(before == after).to_be_true();
	});

	it("uses an indeterminate full-width state for a tempoless first loop", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry({}, 192, 0, 0);
		const auto movingGeometry = ruler::calculateMovingGeometry(staticGeometry, 192, true, 0, true);
		expect(staticGeometry.visible).to_be_true();
		expect(staticGeometry.selectionVisible).to_be_false();
		expect(staticGeometry.markCount).to_equal(uint8_t{0});
		expect(movingGeometry.playheadX).to_equal(uint8_t{127});

		RecordingCanvas canvas;
		ruler::render(canvas, staticGeometry, movingGeometry);
		expect(canvas.lit(64, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(64, OLED_MAIN_TOPMOST_PIXEL + 1)).to_be_false();
		expect(canvas.lit(64, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_false();
	});

	it("moves only the viewport selection when the pad view scrolls", _ {
		const auto atStart = ruler::calculateStaticGeometry(viewport(0, 256), 1024, 128, 512);
		const auto scrolled = ruler::calculateStaticGeometry(viewport(256, 512), 1024, 128, 512);
		expect(atStart.selectionStartX).to_equal(uint8_t{0});
		expect(atStart.selectionEndX).to_equal(uint8_t{31});
		expect(scrolled.selectionStartX).to_equal(uint8_t{32});
		expect(scrolled.selectionEndX).to_equal(uint8_t{63});

		const auto before = ruler::calculateMovingGeometry(atStart, 1024, true, 768);
		const auto after = ruler::calculateMovingGeometry(scrolled, 1024, true, 768);
		expect(before.playheadX).to_equal(uint8_t{96});
		expect(after.playheadX).to_equal(uint8_t{96});
		expect(atStart.markCount).to_equal(scrolled.markCount);
		for (size_t mark = 0; mark < atStart.markCount; mark++) {
			expect(atStart.marks[mark].x).to_equal(scrolled.marks[mark].x);
		}
	});

	it("applies the no-viewport Keyboard policy without losing whole-Clip context", _ {
		const auto timelineViewport = viewport(256, 512);
		const auto keyboardViewport = ruler::viewportForPolicy(ruler::ViewportPolicy::NONE, timelineViewport);
		const auto restoredViewport = ruler::viewportForPolicy(ruler::ViewportPolicy::TIMELINE, timelineViewport);
		expect(keyboardViewport.valid()).to_be_false();
		expect(restoredViewport == timelineViewport).to_be_true();

		const auto timeline = ruler::calculateStaticGeometry(restoredViewport, 1024, 128, 512);
		const auto keyboard = ruler::calculateStaticGeometry(keyboardViewport, 1024, 128, 512);
		expect(timeline.selectionVisible).to_be_true();
		expect(keyboard.visible).to_be_true();
		expect(keyboard.selectionVisible).to_be_false();
		expect(keyboard.markCount).to_equal(timeline.markCount);
		for (size_t mark = 0; mark < keyboard.markCount; mark++) {
			expect(keyboard.marks[mark] == timeline.marks[mark]).to_be_true();
		}

		const auto timelineMoving = ruler::calculateMovingGeometry(timeline, 1024, true, 768);
		const auto keyboardMoving = ruler::calculateMovingGeometry(keyboard, 1024, true, 768);
		expect(keyboardMoving == timelineMoving).to_be_true();

		RecordingCanvas canvas;
		ruler::render(canvas, keyboard, keyboardMoving);
		expect(canvas.lit(0, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(127, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(16, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
		for (int32_t y = OLED_MAIN_TOPMOST_PIXEL; y < OLED_MAIN_TOPMOST_PIXEL + ruler::kStripHeight; y++) {
			expect(canvas.lit(96, y)).to_be_true();
		}
	});

	it("changes viewport width with zoom and spans the ruler for the whole Clip", _ {
		const auto zoomedIn = ruler::calculateStaticGeometry(viewport(0, 128), 1024, 128, 512);
		const auto zoomedOut = ruler::calculateStaticGeometry(viewport(0, 512), 1024, 128, 512);
		const auto wholeClip = ruler::calculateStaticGeometry(viewport(0, 1024), 1024, 128, 512);
		expect(zoomedIn.selectionEndX).to_equal(uint8_t{15});
		expect(zoomedOut.selectionEndX).to_equal(uint8_t{63});
		expect(wholeClip.selectionStartX).to_equal(uint8_t{0});
		expect(wholeClip.selectionEndX).to_equal(uint8_t{127});
	});

	it("clips the viewport selection at both Clip edges", _ {
		const auto left = ruler::calculateStaticGeometry(viewport(-128, 256), 1024, 128, 512);
		const auto right = ruler::calculateStaticGeometry(viewport(768, 1280), 1024, 128, 512);
		const auto beyond = ruler::calculateStaticGeometry(viewport(1024, 1280), 1024, 128, 512);
		const auto reversed = ruler::calculateStaticGeometry(viewport(128, 64), 1024, 128, 512);
		expect(left.selectionVisible).to_be_true();
		expect(left.selectionStartX).to_equal(uint8_t{0});
		expect(left.selectionEndX).to_equal(uint8_t{31});
		expect(right.selectionVisible).to_be_true();
		expect(right.selectionStartX).to_equal(uint8_t{96});
		expect(right.selectionEndX).to_equal(uint8_t{127});
		expect(beyond.selectionVisible).to_be_false();
		expect(reversed.selectionVisible).to_be_false();
		expect(beyond.visible).to_be_true();
	});

	it("keeps a non-empty subpixel viewport selection visible", _ {
		const auto geometry = ruler::calculateStaticGeometry(viewport(1, 2), 1024, 128, 512);
		expect(geometry.selectionVisible).to_be_true();
		expect(geometry.selectionStartX).to_equal(uint8_t{0});
		expect(geometry.selectionEndX).to_equal(uint8_t{0});
	});

	it("uses whole-Clip quarter marks and identifies bars", _ {
		const auto geometry = ruler::calculateStaticGeometry(viewport(256, 512), 1024, 128, 512);
		expect(geometry.markCount).to_equal(uint8_t{8});
		for (size_t mark = 0; mark < geometry.markCount; mark++) {
			expect(geometry.marks[mark].x).to_equal(static_cast<uint8_t>(mark * 16));
			expect(geometry.marks[mark].isBar).to_equal(mark == 0 || mark == 4);
		}
	});

	it("coarsens dense marks by doubling the bar stride", _ {
		const auto geometry = ruler::calculateStaticGeometry(viewport(0, 128), 128, 1, 1);
		expect(geometry.markCount).to_equal(uint8_t{32});
		for (size_t mark = 0; mark < geometry.markCount; mark++) {
			expect(geometry.marks[mark].x).to_equal(static_cast<uint8_t>(mark * 4));
			expect(geometry.marks[mark].isBar).to_be_true();
		}
	});

	it("keeps mark generation bounded at maximum sequence scale", _ {
		const auto geometry = ruler::calculateStaticGeometry(viewport(0, 1600000000), 1600000000, 1, 96);
		expect(geometry.visible).to_be_true();
		expect(geometry.markCount <= ruler::kMaxMarkCount).to_be_true();
		for (size_t mark = 1; mark < geometry.markCount; mark++) {
			expect(geometry.marks[mark].x - geometry.marks[mark - 1].x >= ruler::kMinimumMarkSpacing).to_be_true();
		}
	});

	it("shows precise whole-loop play, wrap, reverse motion, and stopped state", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry(viewport(0, 128), 1024, 128, 512);

		const auto stopped = ruler::calculateMovingGeometry(staticGeometry, 1024, false, 512);
		expect(stopped.visible).to_be_false();

		const auto start = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 0);
		const auto middle = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 512);
		const auto last = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 1023);
		const auto wrapped = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 1024);
		expect(start.playheadX).to_equal(uint8_t{0});
		expect(middle.playheadX).to_equal(uint8_t{64});
		expect(last.playheadX).to_equal(uint8_t{127});
		expect(wrapped.playheadX).to_equal(uint8_t{0});
		expect(last.playheadX > middle.playheadX).to_be_true();
		const auto reverseStart = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 768);
		const auto reverseNext = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 512);
		expect(reverseNext.playheadX < reverseStart.playheadX).to_be_true();
	});

	it("keeps the playhead visible beyond the pad view", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry(viewport(0, 128), 1024, 128, 512);
		const auto beyondView = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 900);
		expect(beyondView.visible).to_be_true();
		expect(beyondView.playheadX).to_equal(uint8_t{112});
	});

	it("maps cloned overdubs for forward, reverse, and Ping-Pong repeats", _ {
		expect(ruler::mapRepeatedPosition(133, 64, ruler::RepeatedDirection::FORWARD)).to_equal(uint32_t{5});
		expect(ruler::mapRepeatedPosition(133, 64, ruler::RepeatedDirection::REVERSE)).to_equal(uint32_t{59});
		expect(ruler::mapRepeatedPosition(69, 64, ruler::RepeatedDirection::PINGPONG)).to_equal(uint32_t{59});
		expect(ruler::mapRepeatedPosition(133, 64, ruler::RepeatedDirection::PINGPONG)).to_equal(uint32_t{5});
		expect(ruler::mapRepeatedPosition(64, 64, ruler::RepeatedDirection::REVERSE)).to_equal(uint32_t{0});
	});

	it("renders progress, selection, marks, caps, and playhead only in the owned strip", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry(viewport(256, 512), 1024, 128, 512);
		const auto movingGeometry = ruler::calculateMovingGeometry(staticGeometry, 1024, true, 768);
		RecordingCanvas canvas;
		ruler::render(canvas, staticGeometry, movingGeometry);

		expect(canvas.outOfBounds).to_be_false();
		for (int32_t y = 0; y < OLED_MAIN_HEIGHT_PIXELS; y++) {
			for (int32_t x = 0; x < OLED_MAIN_WIDTH_PIXELS; x++) {
				if (canvas.lit(x, y)) {
					expect(y >= OLED_MAIN_TOPMOST_PIXEL && y <= OLED_MAIN_TOPMOST_PIXEL + 2).to_be_true();
				}
			}
		}
		expect(canvas.lit(96, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(40, OLED_MAIN_TOPMOST_PIXEL + 1)).to_be_true();
		expect(canvas.lit(64, OLED_MAIN_TOPMOST_PIXEL + 1)).to_be_false();
		expect(canvas.lit(16, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
		expect(canvas.lit(17, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_false();
		expect(canvas.lit(64, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
		expect(canvas.lit(65, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
		for (int32_t y = OLED_MAIN_TOPMOST_PIXEL; y < OLED_MAIN_TOPMOST_PIXEL + ruler::kStripHeight; y++) {
			expect(canvas.lit(96, y)).to_be_true();
		}
	});

	it("shifts a right-edge bar inward to keep it two pixels wide", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry({}, 129, 128, 128);
		expect(staticGeometry.markCount).to_equal(uint8_t{2});
		expect(staticGeometry.marks[1].x).to_equal(uint8_t{127});
		expect(staticGeometry.marks[1].isBar).to_be_true();

		RecordingCanvas canvas;
		ruler::render(canvas, staticGeometry, {});
		expect(canvas.lit(125, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_false();
		expect(canvas.lit(126, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
		expect(canvas.lit(127, OLED_MAIN_TOPMOST_PIXEL + 2)).to_be_true();
	});

	it("keeps the viewport selection while stopped and omits moving progress", _ {
		const auto staticGeometry = ruler::calculateStaticGeometry(viewport(256, 512), 1024, 128, 512);
		RecordingCanvas canvas;
		ruler::render(canvas, staticGeometry, {});

		expect(canvas.lit(0, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(127, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(40, OLED_MAIN_TOPMOST_PIXEL)).to_be_false();
		expect(canvas.lit(40, OLED_MAIN_TOPMOST_PIXEL + 1)).to_be_true();
	});

	it("skips an unchanged cached frame even after its refresh interval", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {.visible = true, .playheadX = 24}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = 100,
			.valid = true,
		};
		const auto decision = ruler::decideRefresh(displayed, displayed.frame, 2305, 2205, false, false);
		expect(decision.draw).to_be_false();
		expect(decision.markDirty).to_be_false();
		expect(decision.checkpointCadence).to_be_true();

		auto refreshed = displayed;
		refreshed.lastRefreshTime = 2305;
		auto moving = displayed.frame;
		moving.movingGeometry.playheadX++;
		const auto nextCallback = ruler::decideRefresh(refreshed, moving, 2966, 2205, false, false);
		expect(nextCallback.draw).to_be_false();
		expect(nextCallback.markDirty).to_be_false();
		expect(nextCallback.checkpointCadence).to_be_false();
	});

	it("rate-limits a moving frame until exactly 2205 samples", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {.visible = true, .playheadX = 24}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = 100,
			.valid = true,
		};
		auto current = displayed.frame;
		current.movingGeometry.playheadX = 25;

		const auto early = ruler::decideRefresh(displayed, current, 2304, 2205, false, false);
		expect(early.draw).to_be_false();
		expect(early.markDirty).to_be_false();
		const auto due = ruler::decideRefresh(displayed, current, 2305, 2205, false, false);
		expect(due.draw).to_be_true();
		expect(due.markDirty).to_be_true();
	});

	it("reuses a cached frame for repeated full renders inside the cadence", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {.visible = true, .playheadX = 24}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = 100,
			.valid = true,
			.transportMoving = true,
			.pinToRightEdge = true,
		};
		expect(ruler::fullRenderProjectionDue(displayed, 101, 2205, false)).to_be_false();
		expect(ruler::fullRenderProjectionDue(displayed, 2304, 2205, false)).to_be_false();
		expect(ruler::fullRenderProjectionDue(displayed, 2305, 2205, false)).to_be_true();
		expect(ruler::fullRenderProjectionDue(displayed, 101, 2205, true)).to_be_true();
		expect(ruler::fullRenderProjectionDue({}, 101, 2205, false)).to_be_true();
	});

	it("draws urgent static and main-canvas replacements immediately", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = 100,
			.valid = true,
		};
		auto staticChange = displayed.frame;
		staticChange.staticGeometryRevision++;
		const auto urgent = ruler::decideRefresh(displayed, staticChange, 101, 2205, true, false);
		expect(urgent.draw).to_be_true();
		expect(urgent.markDirty).to_be_true();

		auto replacement = displayed.frame;
		replacement.mainImageGeneration++;
		const auto replaced = ruler::decideRefresh(displayed, replacement, 101, 2205, false, false);
		expect(replaced.draw).to_be_true();
		expect(replaced.markDirty).to_be_true();
	});

	it("keeps a notification underlay current without requesting a hidden send", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {.visible = true, .playheadX = 24}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = 100,
			.valid = true,
		};
		auto current = displayed.frame;
		current.movingGeometry.playheadX = 25;
		const auto decision = ruler::decideRefresh(displayed, current, 2305, 2205, false, true);
		expect(decision.draw).to_be_true();
		expect(decision.markDirty).to_be_false();
	});

	it("keeps the draw decision correct across the uint32 sample-timer wrap", _ {
		const ruler::RefreshState displayed{
			.frame = {.movingGeometry = {.visible = true, .playheadX = 24}, .staticGeometryRevision = 3, .mainImageGeneration = 7},
			.lastRefreshTime = std::numeric_limits<uint32_t>::max() - 2000,
			.valid = true,
		};
		auto current = displayed.frame;
		current.movingGeometry.playheadX = 25;
		const auto decision = ruler::decideRefresh(displayed, current, 204, 2205, false, false);
		expect(decision.draw).to_be_true();
		expect(decision.markDirty).to_be_true();
	});
});
// clang-format on

CPPSPEC_SPEC(oled_clip_progress_ruler)
