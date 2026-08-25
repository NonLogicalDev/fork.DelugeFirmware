#include "model/clip/audio_clip_bound_edit.h"

#include "cppspec.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace bound_edit = deluge::audio_clip_bound_edit;

namespace {

constexpr bound_edit::GestureAnchor anchor(uint64_t rawStart, uint64_t rawEnd, int32_t loopLength,
                                           uint64_t sourceLength) {
	return {
	    .rawStart = rawStart,
	    .rawEnd = rawEnd,
	    .loopLength = loopLength,
	    .sourceLength = sourceLength,
	};
}

constexpr bound_edit::GestureOffset earlier(uint64_t samples) {
	return {.direction = bound_edit::PlaybackDirection::EARLIER, .samplesFromAnchor = samples};
}

constexpr bound_edit::GestureOffset later(uint64_t samples) {
	return {.direction = bound_edit::PlaybackDirection::LATER, .samplesFromAnchor = samples};
}

} // namespace

// clang-format off
describe audio_clip_bound_edit("Audio Clip bound calculation", $ {
	it("maps forward playback bounds and direction to the matching raw markers", _ {
		const auto start = bound_edit::calculate(anchor(1000, 5000, 800, 6000), bound_edit::PlaybackBound::START,
		                                         false, later(250));
		expect(start).to_have_value();
		expect(start->rawBound).to_equal(bound_edit::RawBound::START);
		expect(start->rawMarker).to_equal(uint64_t{1250});
		expect(start->loopLength).to_equal(750);

		const auto end = bound_edit::calculate(anchor(1000, 5000, 800, 6000), bound_edit::PlaybackBound::END,
		                                       false, later(500));
		expect(end).to_have_value();
		expect(end->rawBound).to_equal(bound_edit::RawBound::END);
		expect(end->rawMarker).to_equal(uint64_t{5500});
		expect(end->loopLength).to_equal(900);
	});

	it("maps reversed playback bounds and direction to the opposite raw markers", _ {
		const auto start = bound_edit::calculate(anchor(1000, 5000, 800, 6000), bound_edit::PlaybackBound::START,
		                                         true, later(250));
		expect(start).to_have_value();
		expect(start->rawBound).to_equal(bound_edit::RawBound::END);
		expect(start->rawMarker).to_equal(uint64_t{4750});
		expect(start->loopLength).to_equal(750);

		const auto end = bound_edit::calculate(anchor(1000, 5000, 800, 6000), bound_edit::PlaybackBound::END,
		                                       true, later(250));
		expect(end).to_have_value();
		expect(end->rawBound).to_equal(bound_edit::RawBound::START);
		expect(end->rawMarker).to_equal(uint64_t{750});
		expect(end->loopLength).to_equal(850);
	});

	it("reaches partial source edges exactly without crossing either bound", _ {
		const auto forwardStart = bound_edit::calculate(anchor(10, 100, 90, 103), bound_edit::PlaybackBound::START,
		                                                false, earlier(1000));
		expect(forwardStart).to_have_value();
		expect(forwardStart->rawMarker).to_equal(uint64_t{0});
		expect(forwardStart->loopLength).to_equal(100);

		const auto forwardEnd = bound_edit::calculate(anchor(10, 100, 90, 103), bound_edit::PlaybackBound::END,
		                                              false, later(1000));
		expect(forwardEnd).to_have_value();
		expect(forwardEnd->rawMarker).to_equal(uint64_t{103});
		expect(forwardEnd->loopLength).to_equal(93);

		const auto reverseStart = bound_edit::calculate(anchor(10, 100, 90, 103), bound_edit::PlaybackBound::START,
		                                                true, earlier(1000));
		expect(reverseStart).to_have_value();
		expect(reverseStart->rawBound).to_equal(bound_edit::RawBound::END);
		expect(reverseStart->rawMarker).to_equal(uint64_t{103});
		expect(reverseStart->loopLength).to_equal(93);

		const auto reverseEnd = bound_edit::calculate(anchor(10, 100, 90, 103), bound_edit::PlaybackBound::END,
		                                              true, later(1000));
		expect(reverseEnd).to_have_value();
		expect(reverseEnd->rawBound).to_equal(bound_edit::RawBound::START);
		expect(reverseEnd->rawMarker).to_equal(uint64_t{0});
		expect(reverseEnd->loopLength).to_equal(100);
	});

	it("uses the gesture anchor ratio instead of compounding detent rounding", _ {
		const auto firstDetent = bound_edit::calculate(anchor(0, 7, 3, 20), bound_edit::PlaybackBound::END,
		                                                false, later(1));
		const auto secondDetent = bound_edit::calculate(anchor(0, 7, 3, 20), bound_edit::PlaybackBound::END,
		                                                 false, later(2));
		expect(firstDetent).to_have_value();
		expect(firstDetent->rawMarker).to_equal(uint64_t{8});
		expect(firstDetent->loopLength).to_equal(3);
		expect(secondDetent).to_have_value();
		expect(secondDetent->rawMarker).to_equal(uint64_t{9});
		expect(secondDetent->loopLength).to_equal(4);
	});

	it("converts cumulative tick distances to samples without repeated rounding or overflow", _ {
		const auto gesture = anchor(10, 17, 3, 30);
		expect(bound_edit::samplesForTickDistance(gesture, 1)).to_equal(uint64_t{2});
		expect(bound_edit::samplesForTickDistance(gesture, 2)).to_equal(uint64_t{5});
		expect(bound_edit::samplesForTickDistance(gesture, 3)).to_equal(uint64_t{7});

		constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
		expect(bound_edit::samplesForTickDistance(anchor(0, maximum, 1, maximum), kMaxSequenceLength))
		    .to_equal(maximum);
	});

	it("derives an exact source-backed negative scroll limit and respects the sequence limit", _ {
		expect(bound_edit::minimumSourceBackedScroll(0, 100, 20)).to_equal(0);
		expect(bound_edit::minimumSourceBackedScroll(11, 100, 20)).to_equal(-3);
		expect(bound_edit::minimumSourceBackedScroll(10, 100, 20)).to_equal(-2);
		expect(bound_edit::minimumSourceBackedScroll(1000, 1, kMaxSequenceLength - 4)).to_equal(-4);
		expect(bound_edit::minimumSourceBackedScroll(std::numeric_limits<uint64_t>::max(),
		                                                    std::numeric_limits<uint64_t>::max(), 1))
		    .to_equal(-1);
	});

	it("saturates a fast gesture at the boundary represented by the changed length", _ {
		expect(bound_edit::realizedTickOffset(bound_edit::PlaybackBound::START, 100, 110)).to_equal(int64_t{-10});
		expect(bound_edit::realizedTickOffset(bound_edit::PlaybackBound::START, 100, 90)).to_equal(int64_t{10});
		expect(bound_edit::realizedTickOffset(bound_edit::PlaybackBound::END, 100, 110)).to_equal(int64_t{10});
		expect(bound_edit::realizedTickOffset(bound_edit::PlaybackBound::END, 100, 90)).to_equal(int64_t{-10});
	});

	it("normalizes a raw-boundary hit to the first tick that reaches its rounded sample", _ {
		const auto gesture = anchor(0, 100, 1000, 110);
		expect(bound_edit::samplesForTickDistance(gesture, 94)).to_equal(uint64_t{9});
		expect(bound_edit::samplesForTickDistance(gesture, 95)).to_equal(uint64_t{10});
		expect(bound_edit::minimumTickDistanceForSamples(gesture, 10)).to_equal(uint32_t{95});
		expect(bound_edit::samplesForTickDistance(gesture, 92)).to_equal(uint64_t{9});
	});

	it("rounds half ticks upward and clamps before a trim would produce zero ticks", _ {
		const auto halfTick = bound_edit::calculate(anchor(0, 2, 1, 2), bound_edit::PlaybackBound::START,
		                                            false, later(1));
		expect(halfTick).to_have_value();
		expect(halfTick->rawMarker).to_equal(uint64_t{1});
		expect(halfTick->loopLength).to_equal(1);

		const auto minimumTick = bound_edit::calculate(anchor(0, 100, 1, 100), bound_edit::PlaybackBound::START,
		                                               false, later(1000));
		expect(minimumTick).to_have_value();
		expect(minimumTick->rawMarker).to_equal(uint64_t{50});
		expect(minimumTick->loopLength).to_equal(1);
	});

	it("clamps expansion to the largest raw duration representable by the sequence", _ {
		const auto result = bound_edit::calculate(anchor(0, 2, kMaxSequenceLength / 2, 100),
		                                          bound_edit::PlaybackBound::END, false, later(100));
		expect(result).to_have_value();
		expect(result->rawMarker).to_equal(uint64_t{4});
		expect(result->loopLength).to_equal(kMaxSequenceLength);
	});

	it("handles maximum-width source positions without multiplication or marker wrap", _ {
		constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
		const auto result = bound_edit::calculate(anchor(1, maximum - 1, 1, maximum),
		                                          bound_edit::PlaybackBound::END, false, later(maximum));
		expect(result).to_have_value();
		expect(result->rawMarker).to_equal(maximum);
		expect(result->loopLength).to_equal(1);
	});

	it("matches native wide arithmetic for boundary-heavy ratio comparisons", _ {
		constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
		constexpr std::array<uint64_t, 5> durations{1, 2, maximum / 2, maximum - 1, maximum};
		constexpr std::array<uint64_t, 4> anchorDurations{1, 3, maximum / 2 + 1, maximum};
		constexpr std::array<uint32_t, 3> anchorLoopLengths{1, 3, static_cast<uint32_t>(kMaxSequenceLength)};
		constexpr std::array<uint32_t, 4> candidates{1, static_cast<uint32_t>(kMaxSequenceLength / 2),
		                                                static_cast<uint32_t>(kMaxSequenceLength),
		                                                static_cast<uint32_t>(kMaxSequenceLength) + 1};

		for (uint64_t duration : durations) {
			for (uint64_t anchorDuration : anchorDurations) {
				for (uint32_t anchorLoopLength : anchorLoopLengths) {
					const unsigned __int128 scaled = static_cast<unsigned __int128>(duration) * anchorLoopLength;
					const unsigned __int128 rounded =
					    (scaled * 2 + anchorDuration) / (static_cast<unsigned __int128>(anchorDuration) * 2);
					for (uint32_t candidate : candidates) {
						expect(bound_edit::detail::roundedLengthAtLeast(duration, anchorLoopLength, anchorDuration,
						                                                        candidate))
						    .to_equal(rounded >= candidate);
					}
				}
			}
		}
	});

	it("rejects invalid anchors, enum values, and requests clamped to the current marker", _ {
		const auto noSource = bound_edit::calculate(anchor(0, 1, 1, 0), bound_edit::PlaybackBound::START,
		                                           false, later(1));
		expect(noSource.error()).to_equal(bound_edit::Rejection::INVALID_SOURCE_LENGTH);

		const auto crossed = bound_edit::calculate(anchor(10, 10, 1, 20), bound_edit::PlaybackBound::START,
		                                          false, later(1));
		expect(crossed.error()).to_equal(bound_edit::Rejection::INVALID_RAW_BOUNDS);

		const auto pastSource = bound_edit::calculate(anchor(0, 21, 1, 20), bound_edit::PlaybackBound::START,
		                                             false, later(1));
		expect(pastSource.error()).to_equal(bound_edit::Rejection::INVALID_RAW_BOUNDS);

		const auto zeroLength = bound_edit::calculate(anchor(0, 1, 0, 1), bound_edit::PlaybackBound::START,
		                                             false, later(1));
		expect(zeroLength.error()).to_equal(bound_edit::Rejection::INVALID_LOOP_LENGTH);

		const auto tooLong = bound_edit::calculate(anchor(0, 1, kMaxSequenceLength + 1, 1),
		                                          bound_edit::PlaybackBound::START, false, later(1));
		expect(tooLong.error()).to_equal(bound_edit::Rejection::INVALID_LOOP_LENGTH);

		const auto invalidBound = bound_edit::calculate(anchor(0, 2, 2, 2),
		                                               static_cast<bound_edit::PlaybackBound>(255), false, later(1));
		expect(invalidBound.error()).to_equal(bound_edit::Rejection::INVALID_PLAYBACK_BOUND);

		const auto invalidDirection = bound_edit::calculate(
		    anchor(0, 2, 2, 2), bound_edit::PlaybackBound::START, false,
		    {.direction = static_cast<bound_edit::PlaybackDirection>(255), .samplesFromAnchor = 1});
		expect(invalidDirection.error()).to_equal(bound_edit::Rejection::INVALID_PLAYBACK_DIRECTION);

		const auto noDistance = bound_edit::calculate(anchor(0, 2, 2, 2), bound_edit::PlaybackBound::START,
		                                            false, later(0));
		expect(noDistance.error()).to_equal(bound_edit::Rejection::NO_CHANGE);

		const auto atSequenceLimit = bound_edit::calculate(anchor(0, 1, kMaxSequenceLength, 100),
		                                                 bound_edit::PlaybackBound::END, false, later(100));
		expect(atSequenceLimit.error()).to_equal(bound_edit::Rejection::NO_CHANGE);
	});
});
// clang-format on

CPPSPEC_SPEC(audio_clip_bound_edit)
