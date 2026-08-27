#include "storage/firmware_compatibility.h"

#include "cppspec.hpp"

namespace compatibility = deluge::firmware_compatibility;

// clang-format off
describe firmware_compatibility("Firmware compatibility", $ {
	it("accepts the supported local save schemas", _ {
		auto current = FirmwareVersion::community({1, 3, 1});
		for (bool ignoreIncorrectFirmware : {false, true}) {
			expect(compatibility::evaluateEarliestCompatibleFirmware("nl-save-schema-0", current,
			                                                           ignoreIncorrectFirmware))
			    .to_equal(Error::NONE);
			expect(compatibility::evaluateEarliestCompatibleFirmware("nl-save-schema-1", current,
			                                                           ignoreIncorrectFirmware))
			    .to_equal(Error::NONE);
		}
	});

	it("rejects empty and malformed local compatibility requirements", _ {
		auto current = FirmwareVersion::community({1, 3, 1});
		for (bool ignoreIncorrectFirmware : {false, true}) {
			for (std::string_view requirement : {
			         "",
			         "n",
			         "nl",
			         "nl-",
			         "nl-save",
			         "nl-save-schem",
			         "nl-save-schema",
			         "nl-save-schema1",
			         "nl-save-schemax-1",
			         "nl-other-1",
			         "nl-save-schema-2",
			         "nl-save-schema-",
			         "nl-save-schema-01",
			         "nl-save-schema-one",
			         "nl-save-schema-1-extra",
			         "nl-save-schema-999999999999999999999999",
			     }) {
				expect(compatibility::evaluateEarliestCompatibleFirmware(requirement, current,
				                                                           ignoreIncorrectFirmware))
				    .to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
			}
		}
	});

	it("preserves ordinary official and Community version comparison", _ {
		auto current = FirmwareVersion::community({1, 3, 1});
		expect(compatibility::evaluateEarliestCompatibleFirmware("4.1.0-alpha", current, false))
		    .to_equal(Error::NONE);
		expect(compatibility::evaluateEarliestCompatibleFirmware("c1.3.1", current, false))
		    .to_equal(Error::NONE);
		expect(compatibility::evaluateEarliestCompatibleFirmware("c1.3.2", current, false))
		    .to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
		expect(compatibility::evaluateEarliestCompatibleFirmware("c1.3.2", current, true))
		    .to_equal(Error::NONE);
	});

});
// clang-format on

CPPSPEC_SPEC(firmware_compatibility)
