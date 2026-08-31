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
#include "util/firmware_version.h"

#include <charconv>
#include <cstdint>
#include <string_view>

namespace deluge::firmware_compatibility {

inline constexpr std::string_view kLocalCompatibilityNamespace = "nl-";
inline constexpr std::string_view kLocalSaveSchemaPrefix = "nl-save-schema-";
inline constexpr std::string_view kLocalSaveSchema1 = "nl-save-schema-1";
inline constexpr std::string_view kLocalSaveSchema2 = "nl-save-schema-2";
inline constexpr uint32_t kSupportedLocalSaveSchema = 2;

[[nodiscard]] inline Error evaluateEarliestCompatibleFirmware(std::string_view requirement,
                                                              FirmwareVersion currentFirmware,
                                                              bool ignoreIncorrectFirmware) {
	if (requirement.empty()) {
		return Error::FILE_FIRMWARE_VERSION_TOO_NEW;
	}

	bool isLocalCompatibilityRequirement =
	    requirement.starts_with(kLocalCompatibilityNamespace) || kLocalCompatibilityNamespace.starts_with(requirement);
	if (isLocalCompatibilityRequirement) {
		if (!requirement.starts_with(kLocalSaveSchemaPrefix)) {
			return Error::FILE_FIRMWARE_VERSION_TOO_NEW;
		}

		std::string_view suffix = requirement.substr(kLocalSaveSchemaPrefix.size());
		if (suffix.empty() || (suffix.size() > 1 && suffix.front() == '0')) {
			return Error::FILE_FIRMWARE_VERSION_TOO_NEW;
		}

		uint32_t schema = 0;
		auto [end, result] = std::from_chars(suffix.data(), suffix.data() + suffix.size(), schema);
		if (result != std::errc{} || end != suffix.data() + suffix.size() || schema > kSupportedLocalSaveSchema) {
			return Error::FILE_FIRMWARE_VERSION_TOO_NEW;
		}
		return Error::NONE;
	}

	FirmwareVersion earliestFirmware = FirmwareVersion::parse(requirement);
	return earliestFirmware > currentFirmware && !ignoreIncorrectFirmware ? Error::FILE_FIRMWARE_VERSION_TOO_NEW
	                                                                      : Error::NONE;
}

} // namespace deluge::firmware_compatibility
