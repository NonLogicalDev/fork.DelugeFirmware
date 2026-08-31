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
inline constexpr std::string_view kChokeGroupsMinimumFirmware = deluge::firmware_compatibility::kLocalSaveSchema2;

enum class LocalSaveSchema : uint8_t {
	NONE = 0,
	EXTERNAL_STEP = 1,
	CHOKE_GROUPS = 2,
};

[[nodiscard]] constexpr LocalSaveSchema maximumRequiredSchema(LocalSaveSchema first, LocalSaveSchema second) {
	return first < second ? second : first;
}

[[nodiscard]] constexpr LocalSaveSchema requiredSchemaForExternalStep(bool containsExternalStepClip) {
	return containsExternalStepClip ? LocalSaveSchema::EXTERNAL_STEP : LocalSaveSchema::NONE;
}

[[nodiscard]] constexpr LocalSaveSchema requiredSchemaForChokeGroup(uint8_t group) {
	return group >= 2 && group <= 16 ? LocalSaveSchema::CHOKE_GROUPS : LocalSaveSchema::NONE;
}

class LocalSaveSchemaRequirement {
public:
	void include(LocalSaveSchema schema) { requiredSchema_ = maximumRequiredSchema(requiredSchema_, schema); }
	void includeExternalStep(bool containsExternalStepClip) {
		include(requiredSchemaForExternalStep(containsExternalStepClip));
	}
	void includeChokeGroup(uint8_t group) { include(requiredSchemaForChokeGroup(group)); }
	[[nodiscard]] LocalSaveSchema value() const { return requiredSchema_; }

private:
	LocalSaveSchema requiredSchema_ = LocalSaveSchema::NONE;
};

/// Returns the oldest reader that understands every local field represented by requiredSchema.
[[nodiscard]] constexpr std::string_view earliestCompatibleFirmware(LocalSaveSchema requiredSchema) {
	switch (requiredSchema) {
	case LocalSaveSchema::NONE:
		return kLegacySongMinimumFirmware;
	case LocalSaveSchema::EXTERNAL_STEP:
		return kExternalStepMinimumFirmware;
	case LocalSaveSchema::CHOKE_GROUPS:
		return kChokeGroupsMinimumFirmware;
	}
	__builtin_unreachable();
}

template <typename Writer>
inline void writeCompatibilityMarker(Writer& writer, LocalSaveSchema requiredSchema) {
	writer.writeEarliestCompatibleFirmwareVersion(earliestCompatibleFirmware(requiredSchema).data());
}

} // namespace deluge::project_compatibility
