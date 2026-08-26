#include "gui/ui/keyboard/layout/velocity_drums_profile.h"

#include "cppspec.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace keyboard = deluge::gui::ui::keyboard;

namespace {

constexpr uint8_t mappedVelocity(keyboard::VelocityDrumsProfile profile, uint32_t x, uint32_t y, uint32_t width,
                                 uint32_t height, uint8_t fixedVelocity = 73, uint8_t fullVelocity = 99) {
	return keyboard::velocityFromDrumsProfile(profile, x, y, width, height, fixedVelocity, fullVelocity);
}

struct Geometry {
	uint32_t width;
	uint32_t height;
};

} // namespace

// clang-format off
describe velocity_drums_profile("Velocity Drums profiles", $ {
	it("defaults invalid persisted fields independently", _ {
		for (int32_t raw = 0; raw <= 3; ++raw) {
			expect(keyboard::isValidVelocityDrumsProfile(raw)).to_be_true();
			expect(static_cast<int32_t>(keyboard::velocityDrumsProfileFromValue(raw))).to_equal(raw);
		}
		for (int32_t raw : std::array<int32_t, 4>{-1, 4, 255, std::numeric_limits<int32_t>::max()}) {
			expect(keyboard::isValidVelocityDrumsProfile(raw)).to_be_false();
			expect(keyboard::velocityDrumsProfileFromValue(raw)).to_equal(keyboard::VelocityDrumsProfile::Full);
		}

		for (int32_t raw : std::array<int32_t, 3>{1, 64, 127}) {
			expect(keyboard::isValidVelocityDrumsFixedVelocity(raw)).to_be_true();
			expect(keyboard::velocityDrumsFixedVelocityFromValue(raw)).to_equal(static_cast<uint8_t>(raw));
		}
		for (int32_t raw : std::array<int32_t, 5>{-1, 0, 128, 255, std::numeric_limits<int32_t>::max()}) {
			expect(keyboard::isValidVelocityDrumsFixedVelocity(raw)).to_be_false();
			expect(keyboard::velocityDrumsFixedVelocityFromValue(raw))
			    .to_equal(keyboard::kDefaultVelocityDrumsFixedVelocity);
		}

		auto profile = keyboard::velocityDrumsProfileFromValue(2);
		auto fixedVelocity = keyboard::velocityDrumsFixedVelocityFromValue(0);
		expect(profile).to_equal(keyboard::VelocityDrumsProfile::Two);
		expect(fixedVelocity).to_equal(keyboard::kDefaultVelocityDrumsFixedVelocity);

		fixedVelocity = keyboard::velocityDrumsFixedVelocityFromValue(77);
		profile = keyboard::velocityDrumsProfileFromValue(99);
		expect(profile).to_equal(keyboard::VelocityDrumsProfile::Full);
		expect(fixedVelocity).to_equal(uint8_t{77});
	});

	it("clamps interactive FIXED edits without changing persisted-field defaults", _ {
		expect(keyboard::clampVelocityDrumsFixedVelocity(-100)).to_equal(uint8_t{1});
		expect(keyboard::clampVelocityDrumsFixedVelocity(0)).to_equal(uint8_t{1});
		expect(keyboard::clampVelocityDrumsFixedVelocity(1)).to_equal(uint8_t{1});
		expect(keyboard::clampVelocityDrumsFixedVelocity(73)).to_equal(uint8_t{73});
		expect(keyboard::clampVelocityDrumsFixedVelocity(127)).to_equal(uint8_t{127});
		expect(keyboard::clampVelocityDrumsFixedVelocity(128)).to_equal(uint8_t{127});
		expect(keyboard::clampVelocityDrumsFixedVelocity(1000)).to_equal(uint8_t{127});
		expect(keyboard::velocityDrumsFixedVelocityFromValue(0))
		    .to_equal(keyboard::kDefaultVelocityDrumsFixedVelocity);
	});

	it("delegates FULL and invalid inputs without altering legacy velocities", _ {
		struct FullCase {
			uint32_t x;
			uint32_t y;
			uint32_t width;
			uint32_t height;
			uint8_t legacyVelocity;
		};
		const std::array cases{
		    FullCase{0, 0, 1, 1, 0},
		    FullCase{1, 0, 2, 1, 7},
		    FullCase{2, 1, 3, 3, 33},
		    FullCase{7, 3, 8, 4, 64},
		    FullCase{15, 7, 16, 8, 100},
		    FullCase{0, 0, 1, 4, 125},
		    FullCase{3, 3, 4, 4, 127},
		};
		for (const auto& test : cases) {
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Full, test.x, test.y, test.width, test.height, 73,
			                      test.legacyVelocity))
			    .to_equal(test.legacyVelocity);
		}

		const auto invalidProfile = static_cast<keyboard::VelocityDrumsProfile>(99);
		expect(mappedVelocity(invalidProfile, 2, 2, 4, 4, 73, 41)).to_equal(uint8_t{41});
		expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, 0, 0, 0, 4, 73, 42)).to_equal(uint8_t{42});
		expect(mappedVelocity(keyboard::VelocityDrumsProfile::Two, 0, 0, 4, 0, 73, 43)).to_equal(uint8_t{43});
	});

	it("maps FOUR across every 4x4 quadrant with the documented orientation", _ {
		constexpr uint8_t expected[4][4] = {
		    {32, 32, 64, 64},
		    {32, 32, 64, 64},
		    {96, 96, 127, 127},
		    {96, 96, 127, 127},
		};
		for (uint32_t y = 0; y < 4; ++y) {
			for (uint32_t x = 0; x < 4; ++x) {
				expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, x, y, 4, 4)).to_equal(expected[y][x]);
			}
		}
	});

	it("maps TWO to bottom and top halves, or left and right on one row", _ {
		for (uint32_t y = 0; y < 4; ++y) {
			for (uint32_t x = 0; x < 4; ++x) {
				uint8_t expected = y < 2 ? 64 : 127;
				expect(mappedVelocity(keyboard::VelocityDrumsProfile::Two, x, y, 4, 4)).to_equal(expected);
			}
		}
		for (uint32_t x = 0; x < 4; ++x) {
			uint8_t expected = x < 2 ? 64 : 127;
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Two, x, 0, 4, 1)).to_equal(expected);
		}
	});

	it("uses one sanitized FIXED velocity throughout each block", _ {
		for (uint32_t y = 0; y < 8; ++y) {
			for (uint32_t x = 0; x < 16; ++x) {
				expect(mappedVelocity(keyboard::VelocityDrumsProfile::Fixed, x, y, 16, 8, 73)).to_equal(uint8_t{73});
			}
		}
		expect(mappedVelocity(keyboard::VelocityDrumsProfile::Fixed, 0, 0, 4, 4, 0))
		    .to_equal(keyboard::kDefaultVelocityDrumsFixedVelocity);
		expect(mappedVelocity(keyboard::VelocityDrumsProfile::Fixed, 0, 0, 4, 4, 128))
		    .to_equal(keyboard::kDefaultVelocityDrumsFixedVelocity);
	});

	it("handles one-cell, single-axis, odd, and wide block geometries", _ {
		for (auto profile : std::array{keyboard::VelocityDrumsProfile::Four, keyboard::VelocityDrumsProfile::Two,
		                               keyboard::VelocityDrumsProfile::Fixed}) {
			expect(mappedVelocity(profile, 0, 0, 1, 1, 73)).to_equal(uint8_t{73});
		}

		constexpr uint8_t fourAxis[4] = {32, 64, 96, 127};
		constexpr uint8_t twoAxis[4] = {64, 64, 127, 127};
		for (uint32_t coordinate = 0; coordinate < 4; ++coordinate) {
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, coordinate, 0, 4, 1))
			    .to_equal(fourAxis[coordinate]);
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, 0, coordinate, 1, 4))
			    .to_equal(fourAxis[coordinate]);
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Two, coordinate, 0, 4, 1))
			    .to_equal(twoAxis[coordinate]);
			expect(mappedVelocity(keyboard::VelocityDrumsProfile::Two, 0, coordinate, 1, 4))
			    .to_equal(twoAxis[coordinate]);
		}

		constexpr uint8_t oddExpected[3][3] = {
		    {32, 32, 64},
		    {32, 32, 64},
		    {96, 96, 127},
		};
		for (uint32_t y = 0; y < 3; ++y) {
			for (uint32_t x = 0; x < 3; ++x) {
				expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, x, y, 3, 3))
				    .to_equal(oddExpected[y][x]);
			}
		}

		for (uint32_t y = 0; y < 4; ++y) {
			for (uint32_t x = 0; x < 8; ++x) {
				uint8_t expected = y < 2 ? (x < 4 ? 32 : 64) : (x < 4 ? 96 : 127);
				expect(mappedVelocity(keyboard::VelocityDrumsProfile::Four, x, y, 8, 4)).to_equal(expected);
			}
		}
	});

	it("never returns zero for a new profile on a valid block", _ {
		constexpr std::array geometries{
		    Geometry{1, 1}, Geometry{4, 1}, Geometry{1, 4}, Geometry{3, 3}, Geometry{8, 4}, Geometry{16, 8},
		};
		constexpr std::array profiles{
		    keyboard::VelocityDrumsProfile::Four,
		    keyboard::VelocityDrumsProfile::Two,
		    keyboard::VelocityDrumsProfile::Fixed,
		};
		for (const auto& geometry : geometries) {
			for (auto profile : profiles) {
				for (uint32_t y = 0; y < geometry.height; ++y) {
					for (uint32_t x = 0; x < geometry.width; ++x) {
						expect(mappedVelocity(profile, x, y, geometry.width, geometry.height, 1) > 0).to_be_true();
					}
				}
			}
		}
	});

	it("derives new-profile brightness from the same mapped velocity", _ {
		constexpr uint32_t cellCount = 64;
		const uint8_t four32 = mappedVelocity(keyboard::VelocityDrumsProfile::Four, 0, 0, 4, 4);
		const uint8_t four64 = mappedVelocity(keyboard::VelocityDrumsProfile::Four, 2, 0, 4, 4);
		const uint8_t four96 = mappedVelocity(keyboard::VelocityDrumsProfile::Four, 0, 2, 4, 4);
		const uint8_t four127 = mappedVelocity(keyboard::VelocityDrumsProfile::Four, 2, 2, 4, 4);
		const uint8_t two64 = mappedVelocity(keyboard::VelocityDrumsProfile::Two, 0, 0, 4, 4);

		expect(keyboard::velocityDrumsIntensityIndex(four32, cellCount)).to_equal(uint32_t{16});
		expect(keyboard::velocityDrumsIntensityIndex(four64, cellCount)).to_equal(uint32_t{32});
		expect(keyboard::velocityDrumsIntensityIndex(four96, cellCount)).to_equal(uint32_t{48});
		expect(keyboard::velocityDrumsIntensityIndex(four127, cellCount)).to_equal(uint32_t{63});
		expect(four64).to_equal(two64);
		expect(keyboard::velocityDrumsIntensityIndex(four64, cellCount))
		    .to_equal(keyboard::velocityDrumsIntensityIndex(two64, cellCount));

		uint32_t previous = 0;
		for (uint8_t velocity = 1; velocity <= 127; ++velocity) {
			uint32_t index = keyboard::velocityDrumsIntensityIndex(velocity, cellCount);
			expect(index >= previous).to_be_true();
			expect(index < cellCount).to_be_true();
			previous = index;
		}
		expect(keyboard::velocityDrumsIntensityIndex(127, 0)).to_equal(uint32_t{0});
		expect(keyboard::velocityDrumsIntensityIndex(127, 1)).to_equal(uint32_t{0});
	});
});
// clang-format on

CPPSPEC_SPEC(velocity_drums_profile)
