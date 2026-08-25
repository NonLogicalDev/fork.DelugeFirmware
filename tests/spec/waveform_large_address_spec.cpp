#include "gui/waveform/waveform_render_data.h"

#include "cppspec.hpp"

#include <cstdint>
#include <limits>

using deluge::gui::waveform::waveformClusterPosition;
using deluge::gui::waveform::waveformSampleBytePosition;
using deluge::gui::waveform::detail::addSampleOffset;

// clang-format off
describe waveform_large_address("large-address waveform rendering", $ {
	it("adds unsigned viewport offsets to signed sample positions without wrapping", _ {
		const auto positive = addSampleOffset(100, 23);
		expect(positive.valid).to_be_true();
		expect(positive.value).to_equal(int64_t{123});

		const auto stillNegative = addSampleOffset(-100, 23);
		expect(stillNegative.valid).to_be_true();
		expect(stillNegative.value).to_equal(int64_t{-77});

		const auto crossesZero = addSampleOffset(-100, 123);
		expect(crossesZero.valid).to_be_true();
		expect(crossesZero.value).to_equal(int64_t{23});

		const auto minimum = addSampleOffset(std::numeric_limits<int64_t>::min(), 0);
		expect(minimum.valid).to_be_true();
		expect(minimum.value).to_equal(std::numeric_limits<int64_t>::min());

		expect(addSampleOffset(std::numeric_limits<int64_t>::max(), 1).valid).to_be_false();
		const auto largest = addSampleOffset(-1, uint64_t{1} << 63);
		expect(largest.valid).to_be_true();
		expect(largest.value).to_equal(std::numeric_limits<int64_t>::max());
		expect(addSampleOffset(-1, std::numeric_limits<uint64_t>::max()).valid).to_be_false();
	});

	it("keeps byte and cluster positions unsigned across 2 GiB and 4 GiB", _ {
		const auto belowTwoGiB = waveformSampleBytePosition(357913933, 6, 49);
		const auto atTwoGiB = waveformSampleBytePosition(357913934, 6, 44);
		expect(belowTwoGiB.valid).to_be_true();
		expect(belowTwoGiB.absoluteByte).to_equal(uint64_t{0x7fffffff});
		expect(atTwoGiB.valid).to_be_true();
		expect(atTwoGiB.absoluteByte).to_equal(uint64_t{0x80000000});
		const auto twoGiBCluster = waveformClusterPosition(atTwoGiB.absoluteByte, 15);
		expect(twoGiBCluster.valid).to_be_true();
		expect(twoGiBCluster.clusterIndex).to_equal(uint64_t{65536});
		expect(twoGiBCluster.byteWithinCluster).to_equal(uint32_t{0});

		const auto belowFourGiB = waveformSampleBytePosition(715827875, 6, 45);
		const auto atFourGiB = waveformSampleBytePosition(715827875, 6, 46);
		expect(belowFourGiB.valid).to_be_true();
		expect(belowFourGiB.absoluteByte).to_equal(uint64_t{0xffffffff});
		expect(atFourGiB.valid).to_be_true();
		expect(atFourGiB.absoluteByte).to_equal(uint64_t{0x100000000});
		const auto fourGiBCluster = waveformClusterPosition(atFourGiB.absoluteByte, 15);
		expect(fourGiBCluster.valid).to_be_true();
		expect(fourGiBCluster.clusterIndex).to_equal(uint64_t{131072});
		expect(fourGiBCluster.byteWithinCluster).to_equal(uint32_t{0});

		const auto beyondInt32 = waveformSampleBytePosition(int64_t{std::numeric_limits<int32_t>::max()} + 1, 1, 0);
		expect(beyondInt32.valid).to_be_true();
		expect(beyondInt32.absoluteByte).to_equal(uint64_t{0x80000000});

		expect(waveformSampleBytePosition(-1, 6, 44).valid).to_be_false();
		expect(waveformSampleBytePosition(1, 0, 44).valid).to_be_false();
		expect(waveformSampleBytePosition(std::numeric_limits<int64_t>::max(), 6, 44).valid).to_be_false();
		expect(waveformClusterPosition(0, 32).valid).to_be_false();
	});
});
// clang-format on

CPPSPEC_SPEC(waveform_large_address)
