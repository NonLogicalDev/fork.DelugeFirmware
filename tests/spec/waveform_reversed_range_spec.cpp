#include "gui/waveform/waveform_render_data.h"

#include "cppspec.hpp"

using deluge::gui::waveform::reverseWaveformColumnRange;

// clang-format off
describe waveform_reversed_range("reversed waveform column ranges", $ {
	it("mirrors full and partial half-open ranges without leaving the display", _ {
		const auto full = reverseWaveformColumnRange(0, kDisplayWidth, kDisplayWidth);
		expect(full.start).to_equal(0);
		expect(full.end).to_equal(kDisplayWidth);

		const auto left = reverseWaveformColumnRange(0, 1, kDisplayWidth);
		expect(left.start).to_equal(kDisplayWidth - 1);
		expect(left.end).to_equal(kDisplayWidth);

		const auto right = reverseWaveformColumnRange(kDisplayWidth - 1, kDisplayWidth, kDisplayWidth);
		expect(right.start).to_equal(0);
		expect(right.end).to_equal(1);

		const auto subset = reverseWaveformColumnRange(2, 5, kDisplayWidth);
		expect(subset.start).to_equal(11);
		expect(subset.end).to_equal(14);
		expect(subset.start >= 0 && subset.end <= kDisplayWidth && subset.start < subset.end).to_be_true();

		const auto widerSubset = reverseWaveformColumnRange(3, 10, kDisplayWidth);
		expect(widerSubset.start).to_equal(6);
		expect(widerSubset.end).to_equal(13);
	});
});
// clang-format on

CPPSPEC_SPEC(waveform_reversed_range)
