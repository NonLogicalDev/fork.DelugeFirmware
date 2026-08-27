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

#include "storage/firmware_compatibility.h"

#include <cstdint>
#include <string_view>

namespace deluge::project_compatibility {

inline constexpr std::string_view kLegacySongMinimumFirmware = "4.1.0-alpha";
inline constexpr std::string_view kExternalStepMinimumFirmware = deluge::firmware_compatibility::kLocalSaveSchema1;

enum class LocalSaveSchema : uint8_t {
	NONE = 0,
	EXTERNAL_STEP = 1,
};

[[nodiscard]] constexpr LocalSaveSchema maximumRequiredSchema(LocalSaveSchema first, LocalSaveSchema second) {
	return first < second ? second : first;
}

[[nodiscard]] constexpr LocalSaveSchema requiredSchemaForExternalStep(bool containsExternalStepClip) {
	return containsExternalStepClip ? LocalSaveSchema::EXTERNAL_STEP : LocalSaveSchema::NONE;
}

/// Older firmware ignores unknown InstrumentClip fields and would run an External Step Clip from Song time.
[[nodiscard]] constexpr std::string_view earliestCompatibleFirmware(LocalSaveSchema requiredSchema) {
	switch (requiredSchema) {
	case LocalSaveSchema::NONE:
		return kLegacySongMinimumFirmware;
	case LocalSaveSchema::EXTERNAL_STEP:
		return kExternalStepMinimumFirmware;
	}
	__builtin_unreachable();
}

} // namespace deluge::project_compatibility
