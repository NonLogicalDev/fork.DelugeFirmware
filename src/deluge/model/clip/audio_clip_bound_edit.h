/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along with The Synthstrom Audible Deluge Firmware.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "definitions_cxx.hpp"

#include <cstdint>
#include <expected>
#include <limits>

namespace deluge::audio_clip_bound_edit {

enum class PlaybackBound : uint8_t {
	START,
	END,
};

/// Direction along playback time, independent of the raw file direction.
enum class PlaybackDirection : uint8_t {
	EARLIER,
	LATER,
};

enum class RawBound : uint8_t {
	START,
	END,
};

enum class Rejection : uint8_t {
	INVALID_SOURCE_LENGTH,
	INVALID_RAW_BOUNDS,
	INVALID_LOOP_LENGTH,
	INVALID_PLAYBACK_BOUND,
	INVALID_PLAYBACK_DIRECTION,
	NO_CHANGE,
};

/// Immutable values captured when one bound-editing gesture begins.
struct GestureAnchor {
	uint64_t rawStart;
	uint64_t rawEnd;
	int32_t loopLength;
	uint64_t sourceLength;
};

/// Cumulative distance from the gesture anchor, not a delta from the preceding detent.
struct GestureOffset {
	PlaybackDirection direction;
	uint64_t samplesFromAnchor;
};

struct Calculation {
	RawBound rawBound;
	uint64_t rawMarker;
	int32_t loopLength;
};

namespace detail {

struct WideProduct {
	uint64_t high;
	uint64_t low;
};

/// Exact 64-by-64-bit multiplication without relying on target support for __int128.
[[nodiscard]] constexpr WideProduct multiplyWide(uint64_t lhs, uint64_t rhs) {
	constexpr uint64_t kLowMask = std::numeric_limits<uint32_t>::max();

	const uint64_t lhsLow = lhs & kLowMask;
	const uint64_t lhsHigh = lhs >> 32;
	const uint64_t rhsLow = rhs & kLowMask;
	const uint64_t rhsHigh = rhs >> 32;

	const uint64_t lowLow = lhsLow * rhsLow;
	const uint64_t lowHigh = lhsLow * rhsHigh;
	const uint64_t highLow = lhsHigh * rhsLow;
	const uint64_t highHigh = lhsHigh * rhsHigh;
	const uint64_t middle = (lowLow >> 32) + (lowHigh & kLowMask) + (highLow & kLowMask);

	return {
	    .high = highHigh + (lowHigh >> 32) + (highLow >> 32) + (middle >> 32),
	    .low = (lowLow & kLowMask) | (middle << 32),
	};
}

[[nodiscard]] constexpr WideProduct doubleWide(WideProduct value) {
	return {
	    .high = (value.high << 1) | (value.low >> 63),
	    .low = value.low << 1,
	};
}

[[nodiscard]] constexpr bool greaterThanOrEqual(WideProduct lhs, WideProduct rhs) {
	return lhs.high > rhs.high || (lhs.high == rhs.high && lhs.low >= rhs.low);
}

/// Tests round(duration * anchorLoopLength / anchorDuration) >= candidate using exact integer products.
[[nodiscard]] constexpr bool roundedLengthAtLeast(uint64_t duration, uint32_t anchorLoopLength, uint64_t anchorDuration,
                                                  uint32_t candidate) {
	if (candidate == 0) {
		return true;
	}

	const WideProduct doubledScaledDuration = doubleWide(multiplyWide(duration, anchorLoopLength));
	const uint64_t thresholdMultiplier = static_cast<uint64_t>(candidate) * 2 - 1;
	const WideProduct threshold = multiplyWide(anchorDuration, thresholdMultiplier);
	return greaterThanOrEqual(doubledScaledDuration, threshold);
}

[[nodiscard]] constexpr uint64_t minimumDurationForOneTick(uint64_t anchorDuration, uint32_t anchorLoopLength) {
	uint64_t low = 1;
	uint64_t high = anchorDuration;
	while (low < high) {
		const uint64_t middle = low + (high - low) / 2;
		if (roundedLengthAtLeast(middle, anchorLoopLength, anchorDuration, 1)) {
			high = middle;
		}
		else {
			low = middle + 1;
		}
	}
	return low;
}

[[nodiscard]] constexpr uint64_t maximumDurationForSequence(uint64_t maximumRawDuration, uint64_t anchorDuration,
                                                            uint32_t anchorLoopLength) {
	constexpr uint32_t kFirstInvalidLoopLength = static_cast<uint32_t>(kMaxSequenceLength) + 1;
	if (!roundedLengthAtLeast(maximumRawDuration, anchorLoopLength, anchorDuration, kFirstInvalidLoopLength)) {
		return maximumRawDuration;
	}

	// The anchor itself is valid, so it is a known-good lower bound for an expansion search.
	uint64_t low = anchorDuration;
	uint64_t high = maximumRawDuration;
	while (low < high) {
		const uint64_t middle = low + (high - low + 1) / 2;
		if (roundedLengthAtLeast(middle, anchorLoopLength, anchorDuration, kFirstInvalidLoopLength)) {
			high = middle - 1;
		}
		else {
			low = middle;
		}
	}
	return low;
}

[[nodiscard]] constexpr int32_t roundedLoopLength(uint64_t duration, uint64_t anchorDuration,
                                                  uint32_t anchorLoopLength) {
	uint32_t low = 1;
	uint32_t high = kMaxSequenceLength;
	while (low < high) {
		const uint32_t middle = low + (high - low + 1) / 2;
		if (roundedLengthAtLeast(duration, anchorLoopLength, anchorDuration, middle)) {
			low = middle;
		}
		else {
			high = middle - 1;
		}
	}
	return static_cast<int32_t>(low);
}

[[nodiscard]] constexpr uint64_t moveRawMarker(uint64_t marker, bool increase, uint64_t distance) {
	if (!increase) {
		return distance > marker ? 0 : marker - distance;
	}

	const uint64_t remaining = std::numeric_limits<uint64_t>::max() - marker;
	return distance > remaining ? std::numeric_limits<uint64_t>::max() : marker + distance;
}

} // namespace detail

/// Converts a cumulative tick distance from a gesture anchor to raw samples without overflowing the intermediate
/// product. The result uses the same nearest-tick rounding rule as calculate().
[[nodiscard]] constexpr uint64_t samplesForTickDistance(const GestureAnchor& anchor, uint32_t tickDistance) {
	if (anchor.loopLength < 1 || anchor.loopLength > kMaxSequenceLength || anchor.rawStart >= anchor.rawEnd) {
		return 0;
	}

	const uint64_t duration = anchor.rawEnd - anchor.rawStart;
	const uint64_t loopLength = static_cast<uint32_t>(anchor.loopLength);
	const uint64_t wholeSamplesPerTick = duration / loopLength;
	const uint64_t remainingSamples = duration % loopLength;

	if (wholeSamplesPerTick != 0 && tickDistance > std::numeric_limits<uint64_t>::max() / wholeSamplesPerTick) {
		return std::numeric_limits<uint64_t>::max();
	}

	const uint64_t whole = wholeSamplesPerTick * tickDistance;
	const uint64_t remainder = (remainingSamples * tickDistance + loopLength / 2) / loopLength;
	if (whole > std::numeric_limits<uint64_t>::max() - remainder) {
		return std::numeric_limits<uint64_t>::max();
	}
	return whole + remainder;
}

/// Returns the smallest cumulative tick distance whose rounded sample distance reaches the requested raw distance.
/// This is the inverse of samplesForTickDistance() at a source boundary, where deriving the offset from the rounded
/// Clip length can leave an encoder gesture beyond the actual boundary.
[[nodiscard]] constexpr uint32_t minimumTickDistanceForSamples(const GestureAnchor& anchor, uint64_t sampleDistance) {
	if (sampleDistance == 0 || anchor.loopLength < 1 || anchor.loopLength > kMaxSequenceLength
	    || anchor.rawStart >= anchor.rawEnd) {
		return 0;
	}

	uint32_t low = 1;
	uint32_t high = kMaxSequenceLength;
	if (samplesForTickDistance(anchor, high) < sampleDistance) {
		return high;
	}

	while (low < high) {
		const uint32_t middle = low + (high - low) / 2;
		if (samplesForTickDistance(anchor, middle) >= sampleDistance) {
			high = middle;
		}
		else {
			low = middle + 1;
		}
	}
	return low;
}

/// Returns the earliest legal Audio Clip scroll position that still maps to real source material before playback
/// Start. Expansion stops at kMaxSequenceLength even when the source file contains more material.
[[nodiscard]] constexpr int32_t minimumSourceBackedScroll(uint64_t recoverableSamples, uint64_t rawDuration,
                                                          int32_t loopLength) {
	if (recoverableSamples == 0 || rawDuration == 0 || loopLength < 1 || loopLength > kMaxSequenceLength) {
		return 0;
	}

	uint32_t low = 0;
	uint32_t high = static_cast<uint32_t>(kMaxSequenceLength - loopLength);
	if (high == 0) {
		return 0;
	}

	const detail::WideProduct required = detail::multiplyWide(recoverableSamples, static_cast<uint32_t>(loopLength));
	if (!detail::greaterThanOrEqual(detail::multiplyWide(rawDuration, high), required)) {
		return -static_cast<int32_t>(high);
	}

	while (low < high) {
		const uint32_t middle = low + (high - low) / 2;
		if (detail::greaterThanOrEqual(detail::multiplyWide(rawDuration, middle), required)) {
			high = middle;
		}
		else {
			low = middle + 1;
		}
	}
	return -static_cast<int32_t>(low);
}

/// Returns the playback-time offset represented by a changed Clip length. This is used when a gesture saturates at a
/// legal source or sequence limit so excess encoder acceleration does not remain in the cumulative input state.
[[nodiscard]] constexpr int64_t realizedTickOffset(PlaybackBound selectedBound, int32_t anchorLoopLength,
                                                   int32_t changedLoopLength) {
	if (selectedBound == PlaybackBound::START) {
		return static_cast<int64_t>(anchorLoopLength) - changedLoopLength;
	}
	return static_cast<int64_t>(changedLoopLength) - anchorLoopLength;
}

/// Calculates one Audio Clip boundary from an immutable gesture anchor.
///
/// Positive playback-time movement is represented by PlaybackDirection::LATER. Reversal changes which raw marker is
/// controlled and which raw direction represents later playback; callers never need to reinterpret raw file order.
[[nodiscard]] constexpr std::expected<Calculation, Rejection>
calculate(const GestureAnchor& anchor, PlaybackBound selectedBound, bool reversed, GestureOffset offset) {
	if (anchor.sourceLength == 0) {
		return std::unexpected(Rejection::INVALID_SOURCE_LENGTH);
	}
	if (anchor.rawStart >= anchor.rawEnd || anchor.rawEnd > anchor.sourceLength) {
		return std::unexpected(Rejection::INVALID_RAW_BOUNDS);
	}
	if (anchor.loopLength < 1 || anchor.loopLength > kMaxSequenceLength) {
		return std::unexpected(Rejection::INVALID_LOOP_LENGTH);
	}

	RawBound rawBound;
	switch (selectedBound) {
	case PlaybackBound::START:
		rawBound = reversed ? RawBound::END : RawBound::START;
		break;
	case PlaybackBound::END:
		rawBound = reversed ? RawBound::START : RawBound::END;
		break;
	default:
		return std::unexpected(Rejection::INVALID_PLAYBACK_BOUND);
	}

	bool movingLater;
	switch (offset.direction) {
	case PlaybackDirection::EARLIER:
		movingLater = false;
		break;
	case PlaybackDirection::LATER:
		movingLater = true;
		break;
	default:
		return std::unexpected(Rejection::INVALID_PLAYBACK_DIRECTION);
	}

	const uint64_t anchorMarker = rawBound == RawBound::START ? anchor.rawStart : anchor.rawEnd;
	const bool increaseRawMarker = movingLater != reversed;
	uint64_t requestedMarker = detail::moveRawMarker(anchorMarker, increaseRawMarker, offset.samplesFromAnchor);

	if (rawBound == RawBound::START) {
		if (requestedMarker >= anchor.rawEnd) {
			requestedMarker = anchor.rawEnd - 1;
		}
	}
	else {
		const uint64_t minimumEnd = anchor.rawStart + 1;
		if (requestedMarker < minimumEnd) {
			requestedMarker = minimumEnd;
		}
		if (requestedMarker > anchor.sourceLength) {
			requestedMarker = anchor.sourceLength;
		}
	}

	const uint64_t anchorDuration = anchor.rawEnd - anchor.rawStart;
	uint64_t requestedDuration =
	    rawBound == RawBound::START ? anchor.rawEnd - requestedMarker : requestedMarker - anchor.rawStart;
	const uint64_t maximumRawDuration =
	    rawBound == RawBound::START ? anchor.rawEnd : anchor.sourceLength - anchor.rawStart;
	const uint64_t minimumDuration = detail::minimumDurationForOneTick(anchorDuration, anchor.loopLength);
	const uint64_t maximumDuration =
	    detail::maximumDurationForSequence(maximumRawDuration, anchorDuration, anchor.loopLength);

	if (requestedDuration < minimumDuration) {
		requestedDuration = minimumDuration;
	}
	if (requestedDuration > maximumDuration) {
		requestedDuration = maximumDuration;
	}

	const uint64_t rawMarker =
	    rawBound == RawBound::START ? anchor.rawEnd - requestedDuration : anchor.rawStart + requestedDuration;
	if (rawMarker == anchorMarker) {
		return std::unexpected(Rejection::NO_CHANGE);
	}

	return Calculation{
	    .rawBound = rawBound,
	    .rawMarker = rawMarker,
	    .loopLength = detail::roundedLoopLength(requestedDuration, anchorDuration, anchor.loopLength),
	};
}

} // namespace deluge::audio_clip_bound_edit
