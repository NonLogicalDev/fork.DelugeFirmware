#pragma once

#include "definitions_cxx.hpp"

#include <cstring>

namespace deluge::song_load {

template <typename Reader>
Error readFirmwareTag(Reader& reader, char const* tagName) {
	Error result = reader.tryReadingFirmwareTagFromFile(tagName, false);
	if (result == Error::NONE) {
		reader.exitTag(tagName);
	}
	return result;
}

// Canonical Songs write the compatibility requirement before their payload, so they still stop after the two leading
// firmware fields. Reordered files must skip root payload until the requirement is found; otherwise an unsupported
// requirement after that payload would be discovered only by the destructive ordinary load. Legacy files without a
// requirement are scanned to the end of the root before being reopened.
template <typename Reader>
Error readRootFirmwareCompatibility(Reader& reader) {
	char const* tagName;
	while (*(tagName = reader.readNextTagOrAttributeName())) {
		bool const isCompatibilityRequirement = !strcmp(tagName, "earliestCompatibleFirmware");
		Error result = readFirmwareTag(reader, tagName);
		if (result == Error::RESULT_TAG_UNUSED) {
			reader.exitTag(tagName);
			continue;
		}
		if (result != Error::NONE) {
			return result;
		}
		if (isCompatibilityRequirement) {
			return Error::NONE;
		}
	}

	return Error::NONE;
}

template <typename Reader>
Error readJsonRootFirmwareCompatibility(Reader& reader) {
	if (!reader.match('{')) {
		return Error::FILE_CORRUPTED;
	}
	return readRootFirmwareCompatibility(reader);
}

// Keep both file opens ahead of every destructive part of Song loading. The second callback must reopen the file
// because the compatibility preflight consumes the root header and closes its reader.
template <typename Preflight, typename Reopen>
Error preflightAndReopen(Preflight&& preflight, Reopen&& reopen) {
	Error result = preflight();
	if (result != Error::NONE) {
		return result;
	}
	return reopen();
}

} // namespace deluge::song_load
