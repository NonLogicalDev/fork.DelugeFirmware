#pragma once

#include <algorithm>
#include <cstdint>

namespace deluge::gui::menu_item::envelope {

// The overview uses legacy 0-50 geometry, independent of a segment's editing precision.
constexpr float graphValue(int32_t value, int32_t maximum) {
	return static_cast<float>(value) * 50.f / maximum;
}

// Display rounding never becomes the source of an edit: retain any off-grid saved value.
constexpr int32_t timeValueHundredths(int32_t raw) {
	return ((static_cast<int64_t>(raw) - INT32_MIN) * 5000 + (int64_t{1} << 31)) >> 32;
}

constexpr int32_t stepTimeValue(int32_t raw, int32_t offset, bool fine) {
	const int64_t delta = static_cast<int64_t>(offset) * 85899345 / (fine ? 100 : 1);
	return std::clamp<int64_t>(static_cast<int64_t>(raw) + delta, INT32_MIN, INT32_MAX);
}

// This is a phase-traversal estimate, not a measurement of block-quantized playback.
constexpr float nominalTimeMilliseconds(int32_t rate, bool attack, int32_t sampleRate) {
	if (attack && rate > 245632) {
		return 0.f;
	}
	return (8388608.0f * 1000.f / sampleRate) / rate;
}

} // namespace deluge::gui::menu_item::envelope
