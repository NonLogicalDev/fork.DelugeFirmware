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
#include "modulation/arpeggiator.h"

#include <cstdint>

namespace deluge::choke_group {

inline constexpr uint8_t kDefault = 1;
inline constexpr uint8_t kMinimum = 1;
inline constexpr uint8_t kMaximum = 16;

[[nodiscard]] constexpr uint8_t normalize(int32_t group) {
	return group >= kMinimum && group <= kMaximum ? group : kDefault;
}

inline void assignNormalized(uint8_t& destination, int32_t group) {
	destination = normalize(group);
}

template <typename WriteAttribute>
inline void writeIfNonDefault(uint8_t group, WriteAttribute&& writeAttribute) {
	group = normalize(group);
	if (group != kDefault) {
		writeAttribute(group);
	}
}

[[nodiscard]] constexpr bool participates(PolyphonyMode mode) {
	return mode == PolyphonyMode::CHOKE;
}

[[nodiscard]] constexpr bool matches(PolyphonyMode candidateMode, uint8_t candidateGroup, uint8_t triggeredGroup) {
	return participates(candidateMode) && normalize(candidateGroup) == normalize(triggeredGroup);
}

[[nodiscard]] inline bool hasResolvedImmediateStart(const ArpReturnInstruction& instruction) {
	return instruction.arpNoteOn != nullptr && instruction.arpNoteOn->noteCodeOnPostArp[0] != ARP_NOTE_NONE;
}

// Choking belongs to a resolved immediate start, not to the input event that may eventually produce one.
template <typename BeforeStart, typename Start>
inline bool performImmediateStart(bool hasResolvedStart, BeforeStart&& beforeStart, Start&& start) {
	if (!hasResolvedStart) {
		return false;
	}
	beforeStart();
	start();
	return true;
}

inline void formatSevenSegmentValue(uint8_t group, char (&text)[4]) {
	group = normalize(group);
	text[0] = 'G';
	text[1] = '0' + group / 10;
	text[2] = '0' + group % 10;
	text[3] = '\0';
}

} // namespace deluge::choke_group
