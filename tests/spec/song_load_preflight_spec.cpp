#include "storage/song_load.h"
#include "storage/storage_manager.h"

#include "cppspec.hpp"

#include <array>
#include <string_view>

namespace {

struct HeaderField {
	std::string_view name;
	Error result;
};

template <size_t size>
class RootHeaderReader {
public:
	explicit RootHeaderReader(std::array<HeaderField, size> fields) : fields_{fields} {}

	char const* readNextTagOrAttributeName() {
		if (next_ == size) {
			return "";
		}
		return fields_[next_++].name.data();
	}

	Error tryReadingFirmwareTagFromFile(char const*, bool) { return fields_[next_ - 1].result; }

	void exitTag(char const*) { ++exited_; }

	[[nodiscard]] size_t fieldsRead() const { return next_; }
	[[nodiscard]] size_t fieldsExited() const { return exited_; }

private:
	std::array<HeaderField, size> fields_;
	size_t next_ = 0;
	size_t exited_ = 0;
};

} // namespace

// clang-format off
describe song_load_preflight("Song load compatibility preflight", $ {
	it("rejects an unsupported XML Song header before reopening or modeled state mutation", _ {
		RootHeaderReader reader{std::array{
		    HeaderField{"firmwareVersion", Error::NONE},
		    HeaderField{"earliestCompatibleFirmware", Error::FILE_FIRMWARE_VERSION_TOO_NEW},
		    HeaderField{"previewNumPads", Error::RESULT_TAG_UNUSED},
		}};
		bool reopened = false;
		bool mutated = false;
		int32_t closeCount = 0;

		Error result = deluge::song_load::preflightAndReopen(
		    [&]() {
			    Error preflightResult = deluge::song_load::readRootFirmwareCompatibility(reader);
			    ++closeCount;
			    return preflightResult;
		    },
		    [&]() {
			    reopened = true;
			    return Error::NONE;
		    });
		if (result == Error::NONE) {
			mutated = true;
		}

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
		expect(closeCount).to_equal(1);
		expect(reopened).to_be_false();
		expect(mutated).to_be_false();
		expect(reader.fieldsRead()).to_equal(size_t{2});
		expect(reader.fieldsExited()).to_equal(size_t{1});
	});

	it("rejects an unsupported reordered XML Song header before reopening or modeled state mutation", _ {
		RootHeaderReader reader{std::array{
		    HeaderField{"previewNumPads", Error::RESULT_TAG_UNUSED},
		    HeaderField{"firmwareVersion", Error::NONE},
		    HeaderField{"earliestCompatibleFirmware", Error::FILE_FIRMWARE_VERSION_TOO_NEW},
		}};
		bool reopened = false;
		bool mutated = false;

		Error result = deluge::song_load::preflightAndReopen(
		    [&]() { return deluge::song_load::readRootFirmwareCompatibility(reader); },
		    [&]() {
			    reopened = true;
			    return Error::NONE;
		    });
		if (result == Error::NONE) {
			mutated = true;
		}

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
		expect(reopened).to_be_false();
		expect(mutated).to_be_false();
		expect(reader.fieldsRead()).to_equal(size_t{3});
		expect(reader.fieldsExited()).to_equal(size_t{2});
	});

	it("reads a compatible JSON Song root with the production in-memory deserializer", _ {
		char json[] = R"({"firmwareVersion":"c1.3.1","earliestCompatibleFirmware":"nl-save-schema-1","previewNumPads":144})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
	});

	it("reads a schema-two JSON Song root with the production in-memory deserializer", _ {
		char json[] = R"({"firmwareVersion":"c1.3.1","earliestCompatibleFirmware":"nl-save-schema-2","previewNumPads":144})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
	});

	it("rejects a future schema JSON Song root with the production in-memory deserializer", _ {
		char json[] = R"({"firmwareVersion":"c1.3.1","earliestCompatibleFirmware":"nl-save-schema-3","previewNumPads":144})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
	});

	it("accepts an empty firmwareVersion as an unknown version without crashing the production reader", _ {
		char json[] = R"({"firmwareVersion":"","earliestCompatibleFirmware":"0.0.0"})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
		expect(song_firmware_version.type() == FirmwareVersion::Type::UNKNOWN).to_be_true();
	});

	it("continues to classify a nonempty invalid firmwareVersion as unknown", _ {
		char json[] = R"({"firmwareVersion":"not-a-version","earliestCompatibleFirmware":"0.0.0"})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
		expect(song_firmware_version.type() == FirmwareVersion::Type::UNKNOWN).to_be_true();
	});

	it("rejects a too-new JSON marker after root payload before reopening or modeled state mutation", _ {
		char json[] = R"({"previewNumPads":144,"preview":{"rows":[1,2,3]},"firmwareVersion":"0.0.0","earliestCompatibleFirmware":"c999.0.0"})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};
		bool reopened = false;
		bool mutated = false;

		Error result = deluge::song_load::preflightAndReopen(
		    [&]() { return deluge::song_load::readJsonRootFirmwareCompatibility(reader); },
		    [&]() {
			    reopened = true;
			    return Error::NONE;
		    });
		if (result == Error::NONE) {
			mutated = true;
		}

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
		expect(reopened).to_be_false();
		expect(mutated).to_be_false();
	});

	it("rejects a too-new JSON marker after nested payload strings containing structural characters", _ {
		char json[] = R"json({"preview":{"label":"literal } and ] with quote \" and slash \\","rows":[{"label":"nested [ and {"}]},"firmwareVersion":"0.0.0","earliestCompatibleFirmware":"c999.0.0"})json";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
	});

	it("rejects a too-new JSON marker after an escaped string payload", _ {
		char json[] = R"json({"label":"literal } and ] with quote \" and slash \\","firmwareVersion":"0.0.0","earliestCompatibleFirmware":"c999.0.0"})json";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
	});

	it("accepts compatible JSON after nested payload strings containing structural characters", _ {
		char json[] = R"json({"preview":{"label":"literal } and ] with quote \" and slash \\","rows":[{"label":"nested [ and {"}]},"firmwareVersion":"0.0.0","earliestCompatibleFirmware":"0.0.0"})json";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
	});

	it("accepts a compatible reordered JSON Song root with the production in-memory deserializer", _ {
		char json[] = R"({"previewNumPads":144,"preview":[{"row":1},{"row":2}],"firmwareVersion":"0.0.0","earliestCompatibleFirmware":"0.0.0"})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
	});

	it("accepts a legacy JSON Song root without compatibility fields", _ {
		char json[] = R"({"previewNumPads":144,"preview":[{"row":1},{"row":2}]})";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::NONE);
	});

	it("rejects JSON input without an inner Song object root", _ {
		char json[] = R"("firmwareVersion":"c1.2.0")";
		JsonDeserializer reader{reinterpret_cast<uint8_t*>(json), sizeof(json) - 1};

		Error result = deluge::song_load::readJsonRootFirmwareCompatibility(reader);

		expect(result).to_equal(Error::FILE_CORRUPTED);
	});

	it("closes a compatible Song preflight before reopening and modeled state mutation", _ {
		RootHeaderReader reader{std::array{
		    HeaderField{"firmwareVersion", Error::NONE},
		    HeaderField{"earliestCompatibleFirmware", Error::NONE},
		    HeaderField{"previewNumPads", Error::RESULT_TAG_UNUSED},
		}};
		bool reopened = false;
		bool reopenedBeforeClose = false;
		bool mutated = false;
		int32_t closeCount = 0;

		Error result = deluge::song_load::preflightAndReopen(
		    [&]() {
			    Error preflightResult = deluge::song_load::readRootFirmwareCompatibility(reader);
			    ++closeCount;
			    return preflightResult;
		    },
		    [&]() {
			    reopenedBeforeClose = closeCount != 1;
			    reopened = true;
			    return Error::NONE;
		    });
		if (result == Error::NONE) {
			mutated = true;
		}

		expect(result).to_equal(Error::NONE);
		expect(closeCount).to_equal(1);
		expect(reopened).to_be_true();
		expect(reopenedBeforeClose).to_be_false();
		expect(mutated).to_be_true();
		expect(reader.fieldsRead()).to_equal(size_t{2});
		expect(reader.fieldsExited()).to_equal(size_t{2});
	});

	it("preserves an ordinary reopen error", _ {
		bool reopened = false;
		Error result = deluge::song_load::preflightAndReopen(
		    []() { return Error::NONE; },
		    [&]() {
			    reopened = true;
			    return Error::FILE_CORRUPTED;
		    });

		expect(reopened).to_be_true();
		expect(result).to_equal(Error::FILE_CORRUPTED);
	});

	it("propagates an unsupported firmware tag without consuming it as compatible", _ {
		RootHeaderReader reader{
		    std::array{HeaderField{"earliestCompatibleFirmware", Error::FILE_FIRMWARE_VERSION_TOO_NEW}},
		};
		char const* tagName = reader.readNextTagOrAttributeName();

		Error result = deluge::song_load::readFirmwareTag(reader, tagName);

		expect(result).to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
		expect(reader.fieldsExited()).to_equal(size_t{0});
	});
});

CPPSPEC_SPEC(song_load_preflight)
