#include "storage/cluster/cluster.h"
#include "storage/storage_manager.h"
#include "util/d_string.h"

#include <cstdlib>

// Host specs exercise the production JsonDeserializer. These definitions provide the memory-reader and otherwise
// unused firmware dependencies that the full target normally gets from StorageManager, FatFS, and the audio engine.

size_t Cluster::size = Cluster::kSizeFAT16Max;
FirmwareVersion song_firmware_version = FirmwareVersion::current();

namespace AudioEngine {
void logAudioAction(char const*, char const*, int) {
}
} // namespace AudioEngine

uint32_t hexToInt(char const* string) {
	return static_cast<uint32_t>(strtoul(string, nullptr, 16));
}

bool getNibble(char ch, int* nibble) {
	if (ch >= '0' && ch <= '9') {
		*nibble = ch - '0';
		return true;
	}
	if (ch >= 'A' && ch <= 'F') {
		*nibble = ch - 'A' + 10;
		return true;
	}
	if (ch >= 'a' && ch <= 'f') {
		*nibble = ch - 'a' + 10;
		return true;
	}
	return false;
}

void String::clear(bool) {
}

Error String::concatenateAtPos(char const*, int32_t, int32_t) {
	return Error::NONE;
}

FileReader::FileReader()
    : fileClusterBuffer{nullptr}, currentReadBufferEndPos{0}, fileReadBufferCurrentPos{0}, memoryBased{true},
      callRoutines{false} {
}

FileReader::FileReader(char* memBuffer, uint32_t bufLen)
    : fileClusterBuffer{memBuffer}, currentReadBufferEndPos{bufLen}, fileReadBufferCurrentPos{0}, memoryBased{true},
      callRoutines{false} {
}

FileReader::~FileReader() = default;

void FileReader::resetReader() {
	fileReadBufferCurrentPos = 0;
	readCount = 0;
	reachedBufferEnd = false;
}

bool FileReader::readFileCluster() {
	return false;
}

bool FileReader::readFileClusterIfNecessary() {
	if (fileReadBufferCurrentPos >= currentReadBufferEndPos) {
		reachedBufferEnd = true;
	}
	return !reachedBufferEnd;
}

bool FileReader::peekChar(char* thisChar) {
	if (!readFileClusterIfNecessary()) {
		return false;
	}
	*thisChar = fileClusterBuffer[fileReadBufferCurrentPos];
	return true;
}

bool FileReader::readChar(char* thisChar) {
	if (!peekChar(thisChar)) {
		return false;
	}
	++fileReadBufferCurrentPos;
	return true;
}

void FileReader::readDone() {
}
