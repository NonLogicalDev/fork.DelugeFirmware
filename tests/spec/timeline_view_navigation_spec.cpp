#include "gui/views/timeline_view_navigation.h"

#include "cppspec.hpp"

#include <cstdint>
#include <limits>

namespace navigation = deluge::gui::timeline_view_navigation;

// clang-format off
describe timeline_view_navigation("Timeline navigation arithmetic", $ {
	it("clamps signed scroll requests without wrapping", _ {
		expect(navigation::clampScroll(-51, -50)).to_equal(-50);
		expect(navigation::clampScroll(-49, -50)).to_equal(-49);
		expect(navigation::clampScroll(int64_t{std::numeric_limits<int32_t>::max()} + 1, -50))
		    .to_equal(std::numeric_limits<int32_t>::max());
	});

	it("uses mathematical floor when aligning negative scroll for zoom", _ {
		expect(navigation::floorToMultiple(31, 16)).to_equal(16);
		expect(navigation::floorToMultiple(0, 16)).to_equal(0);
		expect(navigation::floorToMultiple(-1, 16)).to_equal(-16);
		expect(navigation::floorToMultiple(-16, 16)).to_equal(-16);
		expect(navigation::floorToMultiple(-17, 16)).to_equal(-32);
	});

	it("aligns to the source-backed minimum when the preceding block is outside it", _ {
		expect(navigation::alignScrollForZoom(-17, 16, -20)).to_equal(-20);
		expect(navigation::alignScrollForZoom(-17, 16, -40)).to_equal(-32);
	});

	it("keeps the same source position anchored when Start rebases to tick zero", _ {
		expect(navigation::reanchorScrollAfterStartEdit(-8, 100, 110, -20)).to_equal(2);
		expect(navigation::reanchorScrollAfterStartEdit(0, 100, 90, -25)).to_equal(-10);
		expect(navigation::reanchorScrollAfterStartEdit(-18, 100, 90, -20)).to_equal(-20);
	});
});
// clang-format on

CPPSPEC_SPEC(timeline_view_navigation)
