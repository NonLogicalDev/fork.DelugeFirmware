#include "gui/waveform/waveform_playhead.h"

#include "cppspec.hpp"

#include <array>
#include <cstdint>

namespace waveform = deluge::gui::waveform;

namespace {

Sample const* sampleIdentity(uintptr_t value) {
	return reinterpret_cast<Sample const*>(value);
}

waveform::OledWaveformPlayheadInput input(Sample const* displayed, Sample const* playing, int64_t position,
                                          int64_t scroll = 100, int64_t zoom = 10) {
	return {
	    .displayedSample = displayed,
	    .playingSample = playing,
	    .samplePosition = position,
	    .xScroll = scroll,
	    .xZoom = zoom,
	};
}

class InvertingCanvas {
public:
	void invertArea(int32_t x, int32_t width, int32_t top, int32_t bottom) {
		for (int32_t y = top; y <= bottom; y++) {
			for (int32_t column = x; column < x + width; column++) {
				pixels[y][column] = !pixels[y][column];
			}
		}
	}

	std::array<std::array<bool, OLED_MAIN_WIDTH_PIXELS>, OLED_MAIN_HEIGHT_PIXELS> pixels{};
};

} // namespace

static_assert(waveform::padWaveformBrightness(false, waveform::PadWaveformIntensity::FULL) == 128);
static_assert(waveform::padWaveformBrightness(false, waveform::PadWaveformIntensity::PLAYHEAD_FOCUSED) == 112);
static_assert(waveform::padWaveformBrightness(true, waveform::PadWaveformIntensity::FULL) == 256);
static_assert(waveform::padWaveformBrightness(true, waveform::PadWaveformIntensity::PLAYHEAD_FOCUSED) == 224);
static_assert(waveform::padWaveformIntensityForRenderTarget(true) == waveform::PadWaveformIntensity::PLAYHEAD_FOCUSED);
static_assert(waveform::padWaveformIntensityForRenderTarget(false) == waveform::PadWaveformIntensity::FULL);
static_assert(waveform::isWaveformPlayheadCandidate(true, true, true));
static_assert(!waveform::isWaveformPlayheadCandidate(false, true, true));
static_assert(!waveform::isWaveformPlayheadCandidate(true, false, true));
static_assert(!waveform::isWaveformPlayheadCandidate(true, true, false));

// clang-format off
describe waveform_playhead("Waveform playhead", $ {
	it("maps exact sample ownership across the OLED viewport", _ {
		auto const* sampleA = sampleIdentity(1);
		auto const* sampleB = sampleIdentity(2);
		waveform::OledWaveformPlayheadState state;

		expect(state.update(input(sampleA, sampleB, 100), 0)).to_be_false();
		expect(state.visibleColumn(sampleA, 100, 10)).to_equal(-1);
		expect(state.update(input(sampleA, sampleA, 100), 1)).to_be_true();
		expect(state.visibleColumn(sampleA, 100, 10)).to_equal(0);

		state.reset();
		expect(state.update(input(sampleA, sampleA, 259), 2)).to_be_true();
		expect(state.visibleColumn(sampleA, 100, 10)).to_equal(127);
		expect(state.visibleColumn(sampleB, 100, 10)).to_equal(-1);
	});

	it("suppresses offscreen sources and cleans a stopped cursor once", _ {
		auto const* sample = sampleIdentity(1);
		waveform::OledWaveformPlayheadState state;
		const uint32_t interval = waveform::kOledWaveformPlayheadRefreshInterval;

		expect(state.update(input(sample, sample, 99), 0)).to_be_false();
		expect(state.update(input(sample, sample, 120), 1)).to_be_true();
		expect(state.visibleColumn(sample, 100, 10)).to_equal(16);
		expect(state.update(input(sample, nullptr, 0), interval)).to_be_false();
		expect(state.update(input(sample, nullptr, 0), interval + 1)).to_be_true();
		expect(state.visibleColumn(sample, 100, 10)).to_equal(-1);
		expect(state.update(input(sample, nullptr, 0), interval + 2)).to_be_false();
	});

	it("caps movement at twenty hertz and ignores an unchanged pixel", _ {
		auto const* sample = sampleIdentity(1);
		waveform::OledWaveformPlayheadState state;
		const uint32_t interval = waveform::kOledWaveformPlayheadRefreshInterval;

		expect(state.update(input(sample, sample, 120), 100)).to_be_true();
		expect(state.update(input(sample, sample, 121), 100 + interval)).to_be_false();
		expect(state.update(input(sample, sample, 140), 100 + interval - 1)).to_be_false();
		expect(state.visibleColumn(sample, 100, 10)).to_equal(16);
		expect(state.update(input(sample, sample, 140), 100 + interval)).to_be_true();
		expect(state.visibleColumn(sample, 100, 10)).to_equal(32);
	});

	it("hides a stale column until a changed viewport is committed", _ {
		auto const* sample = sampleIdentity(1);
		waveform::OledWaveformPlayheadState state;
		const uint32_t interval = waveform::kOledWaveformPlayheadRefreshInterval;

		expect(state.update(input(sample, sample, 180), 0)).to_be_true();
		expect(state.visibleColumn(sample, 100, 10)).to_equal(64);
		expect(state.visibleColumn(sample, 180, 10)).to_equal(-1);
		expect(state.update(input(sample, sample, 180, 180, 10), interval)).to_be_true();
		expect(state.visibleColumn(sample, 180, 10)).to_equal(0);
	});

	it("inverts one column after existing waveform and marker pixels", _ {
		auto const* sample = sampleIdentity(1);
		waveform::OledWaveformPlayheadState state;
		expect(state.update(input(sample, sample, 120), 0)).to_be_true();

		InvertingCanvas canvas;
		canvas.pixels[20][16] = true;
		waveform::renderOledWaveformPlayhead(canvas, state, sample, 100, 10, 18, 22);
		expect(canvas.pixels[18][16]).to_be_true();
		expect(canvas.pixels[20][16]).to_be_false();
		expect(canvas.pixels[22][16]).to_be_true();
		expect(canvas.pixels[20][15]).to_be_false();
		expect(canvas.pixels[20][17]).to_be_false();
	});
});

CPPSPEC_SPEC(waveform_playhead)
