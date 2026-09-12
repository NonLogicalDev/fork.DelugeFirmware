#pragma once

#include "model/scale/preset_scales.h"

// The initiating release belongs to this gesture even if the menu has already closed.
class ScaleMenuGesture {
public:
	enum class Result { PASS, CONSUME, OPEN, DEFER };
	Result handle(bool on, bool eligible, bool inCardRoutine) {
		if (ownsRelease_) {
			if (!on) {
				ownsRelease_ = false;
			}
			return Result::CONSUME;
		}
		if (!on || !eligible) {
			return Result::PASS;
		}
		if (inCardRoutine) {
			return Result::DEFER;
		}
		ownsRelease_ = true;
		return Result::OPEN;
	}

private:
	bool ownsRelease_ = false;
};

// Step only through selectable modes; retaining an unlisted current mode is a valid no-op.
inline Scale stepScaleMenuMode(Scale current, int8_t offset, const std::bitset<NUM_ALL_SCALES>& available) {
	if (!offset || available.none()) {
		return current;
	}
	int candidate = current < NUM_ALL_SCALES ? current : (offset > 0 ? NUM_ALL_SCALES - 1 : 0);
	const int direction = offset > 0 ? 1 : -1;
	const int steps = offset > 0 ? offset : -static_cast<int>(offset);
	for (int step = 0; step < steps; ++step) {
		do {
			candidate = (candidate + direction + NUM_ALL_SCALES) % NUM_ALL_SCALES;
		} while (!available[candidate]);
	}
	return static_cast<Scale>(candidate);
}
