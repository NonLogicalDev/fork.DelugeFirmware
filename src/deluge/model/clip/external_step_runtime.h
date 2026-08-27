/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "definitions_cxx.hpp"
#include <array>
#include <cstdint>

namespace deluge::external_step {

enum class ClockMode : uint8_t {
	SONG,
	EXTERNAL_STEP,
};

enum class StepSize : uint8_t {
	QUARTER,
	EIGHTH,
	SIXTEENTH,
	THIRTY_SECOND,
};

enum class State : uint8_t {
	PRE_ZERO,
	RUNNING,
	WAIT_CONTINUE,
};

enum class Eligibility : uint8_t {
	ELIGIBLE,
	SONG_MODE,
	FEATURE_DISABLED,
	NOT_MIDI_OUT,
	ARRANGEMENT_ONLY,
	RECORDING,
	ARPEGGIATOR,
	UNSUPPORTED_DIRECTION,
	INDIVISIBLE_LOOP,
	INDEPENDENT_NOTE_ROW,
	STEP_UNASSIGNED,
	STEP_INPUT_MISSING,
	CONFLICT,
	DUPLICATE_BINDING,
};

enum class Status : uint8_t {
	SONG,
	FEATURE_DISABLED,
	UNASSIGNED,
	MISSING_INPUT,
	CONFLICT,
	UNSUPPORTED,
	WAIT,
	READY,
};

struct Advance {
	bool emit = false;
	bool wrapped = false;
	uint32_t position = 0;
};

[[nodiscard]] constexpr StepSize stepSizeFromValue(int32_t value) {
	switch (value) {
	case 0:
		return StepSize::QUARTER;
	case 1:
		return StepSize::EIGHTH;
	case 3:
		return StepSize::THIRTY_SECOND;
	case 2:
	default:
		return StepSize::SIXTEENTH;
	}
}

[[nodiscard]] constexpr uint32_t baseTicksForStepSize(StepSize size) {
	switch (size) {
	case StepSize::QUARTER:
		return 24;
	case StepSize::EIGHTH:
		return 12;
	case StepSize::THIRTY_SECOND:
		return 3;
	case StepSize::SIXTEENTH:
	default:
		return 6;
	}
}

/// Quantize a stored note end without rewriting the note. The note start has already been delayed to the emitted
/// boundary. A stored end that would fall at or before that boundary is extended to the next pulse.
[[nodiscard]] constexpr uint32_t noteDurationInPulses(uint32_t notePosition, uint32_t noteLength,
                                                      uint32_t boundaryPosition, uint32_t loopLength,
                                                      uint32_t stepTicks, bool wrapped) {
	if (stepTicks == 0 || loopLength == 0) {
		return 1;
	}

	uint64_t boundary = boundaryPosition;
	uint64_t noteStart = notePosition;
	if (wrapped) {
		boundary += loopLength;
		if (notePosition <= boundaryPosition) {
			noteStart += loopLength;
		}
	}

	const uint64_t noteEnd = noteStart + noteLength;
	const uint64_t quantizedEnd = ((noteEnd + stepTicks - 1) / stepTicks) * stepTicks;
	if (quantizedEnd <= boundary) {
		return 1;
	}
	const uint64_t pulses = (quantizedEnd - boundary) / stepTicks;
	return pulses == 0 ? 1 : static_cast<uint32_t>(pulses);
}

/// Fixed-size local phase and watchdog state for one externally stepped Clip.
///
/// This object deliberately knows nothing about the Song scheduler or MIDI dispatch. Callers provide the current
/// Clip position and apply the returned boundary synchronously at the accepted control edge.
class Runtime {
public:
	static constexpr uint32_t kMinimumStableInterval = kSampleRate / 40; // 25 ms
	static constexpr uint32_t kMaximumStableInterval = kSampleRate * 4;  // 4 seconds

	[[nodiscard]] Advance advance(uint32_t currentPosition, uint32_t loopLength, uint32_t stepTicks,
	                              uint32_t sampleTime) {
		Advance result;
		if (loopLength == 0 || stepTicks == 0 || loopLength % stepTicks != 0) {
			return result;
		}

		result.emit = true;
		if (state_ == State::PRE_ZERO) {
			result.position = 0;
		}
		else {
			const uint32_t next = currentPosition + stepTicks;
			result.wrapped = next >= loopLength;
			result.position = result.wrapped ? 0 : next;
		}

		state_ = State::RUNNING;
		observePulse(sampleTime);
		return result;
	}

	void resetToPreZero() {
		state_ = State::PRE_ZERO;
		clearCadence();
	}

	void waitPreservingPhase() {
		if (state_ != State::PRE_ZERO) {
			state_ = State::WAIT_CONTINUE;
		}
		clearCadence();
	}

	[[nodiscard]] bool timeoutDue(uint32_t sampleTime) const {
		return timeoutArmed_ && static_cast<int32_t>(sampleTime - timeoutDeadline_) >= 0;
	}

	[[nodiscard]] bool hasTimeoutDeadline() const { return timeoutArmed_; }
	[[nodiscard]] uint32_t timeoutDeadline() const { return timeoutDeadline_; }
	[[nodiscard]] State state() const { return state_; }
	[[nodiscard]] bool isWaiting() const { return state_ != State::RUNNING; }

private:
	static constexpr uint32_t median(std::array<uint32_t, 3> values) {
		if (values[0] > values[1]) {
			const uint32_t value = values[0];
			values[0] = values[1];
			values[1] = value;
		}
		if (values[1] > values[2]) {
			const uint32_t value = values[1];
			values[1] = values[2];
			values[2] = value;
		}
		if (values[0] > values[1]) {
			values[1] = values[0];
		}
		return values[1];
	}

	static constexpr bool cadenceIsStable(const std::array<uint32_t, 3>& intervals, uint32_t expected) {
		if (expected < kMinimumStableInterval || expected > kMaximumStableInterval) {
			return false;
		}
		for (uint32_t interval : intervals) {
			const uint32_t difference = interval > expected ? interval - expected : expected - interval;
			if (static_cast<uint64_t>(difference) * 4 > expected) {
				return false;
			}
		}
		return true;
	}

	void observePulse(uint32_t sampleTime) {
		if (haveLastPulse_) {
			intervals_[nextInterval_] = sampleTime - lastPulse_;
			nextInterval_ = (nextInterval_ + 1) % intervals_.size();
			if (intervalCount_ < intervals_.size()) {
				intervalCount_++;
			}

			if (intervalCount_ == intervals_.size()) {
				const uint32_t expected = median(intervals_);
				if (cadenceIsStable(intervals_, expected)) {
					timeoutDeadline_ = sampleTime + expected * 4;
					timeoutArmed_ = true;
				}
				else {
					timeoutArmed_ = false;
				}
			}
		}

		lastPulse_ = sampleTime;
		haveLastPulse_ = true;
	}

	void clearCadence() {
		intervals_ = {};
		lastPulse_ = 0;
		timeoutDeadline_ = 0;
		nextInterval_ = 0;
		intervalCount_ = 0;
		haveLastPulse_ = false;
		timeoutArmed_ = false;
	}

	std::array<uint32_t, 3> intervals_{};
	uint32_t lastPulse_ = 0;
	uint32_t timeoutDeadline_ = 0;
	uint8_t nextInterval_ = 0;
	uint8_t intervalCount_ = 0;
	bool haveLastPulse_ = false;
	bool timeoutArmed_ = false;
	State state_ = State::PRE_ZERO;
};

} // namespace deluge::external_step
