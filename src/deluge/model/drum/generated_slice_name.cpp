/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "model/drum/generated_slice_name.h"

#include <limits>

namespace deluge::generated_slice_name {

namespace {

bool isAsciiLetter(char character) {
	return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

int32_t letterValue(char character) {
	if (character >= 'a' && character <= 'z') {
		character -= 'a' - 'A';
	}
	return character - 'A' + 1;
}

} // namespace

int32_t getSeriesIndex(std::string_view name) {
	uint32_t series = 0;
	size_t position = 0;
	while (position < name.size() && isAsciiLetter(name[position])) {
		uint32_t letter = letterValue(name[position]);
		if (series > (std::numeric_limits<int32_t>::max() - letter) / 26) {
			return -1;
		}
		series = series * 26 + letter;
		position++;
	}

	if (position == 0 || position == name.size() || name[position++] != '-') {
		return -1;
	}

	size_t partStart = position;
	uint32_t partNumber = 0;
	while (position < name.size() && name[position] >= '0' && name[position] <= '9') {
		if (partNumber > 256 / 10) {
			return -1;
		}
		partNumber = partNumber * 10 + (name[position] - '0');
		if (partNumber > 256) {
			return -1;
		}
		position++;
	}

	if (position != name.size() || partNumber == 0) {
		return -1;
	}

	size_t canonicalPartLength = partNumber < 100 ? 2 : 3;
	if (position - partStart != canonicalPartLength) {
		return -1;
	}

	return series - 1;
}

bool isGeneratedSliceName(std::string_view name) {
	return getSeriesIndex(name) >= 0;
}

std::string makeName(int32_t seriesIndex, int32_t partNumber) {
	std::string name;
	uint32_t series = seriesIndex + 1;
	while (series) {
		uint32_t next = (series - 1) % 26;
		name.insert(name.begin(), static_cast<char>('A' + next));
		series = (series - 1) / 26;
	}

	name += '-';
	if (partNumber < 10) {
		name += '0';
	}
	name += std::to_string(partNumber);
	return name;
}

} // namespace deluge::generated_slice_name
