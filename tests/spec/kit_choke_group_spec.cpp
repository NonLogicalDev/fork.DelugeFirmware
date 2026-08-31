#include "gui/l10n/l10n.h"
#include "gui/menu_item/voice/polyphony_policy.h"
#include "model/song/project_compatibility.h"
#include "processing/sound/choke_group.h"
#include "storage/song_load.h"
#include "storage/storage_manager.h"

#include "cppspec.hpp"

#include <array>
#include <string>
#include <string_view>

namespace choke_group = deluge::choke_group;
namespace compatibility = deluge::project_compatibility;
namespace polyphony_policy = deluge::gui::menu_item::voice::polyphony_policy;

namespace {

struct CompatibilityMarkerWriter {
	void writeEarliestCompatibleFirmwareVersion(char const* value) { marker = value; }
	std::string marker;
};

} // namespace

// clang-format off
describe kit_choke_group("Kit choke groups", $ {
	it("applies the Sound Drum default and normalized read and clone assignments", _ {
		uint8_t loadedGroup = choke_group::kDefault;
		expect(loadedGroup).to_equal(uint8_t{1});

		choke_group::assignNormalized(loadedGroup, 0);
		expect(loadedGroup).to_equal(uint8_t{1});
		choke_group::assignNormalized(loadedGroup, 16);
		expect(loadedGroup).to_equal(uint8_t{16});
		choke_group::assignNormalized(loadedGroup, 17);
		expect(loadedGroup).to_equal(uint8_t{1});

		uint8_t clonedGroup = 9;
		choke_group::assignNormalized(clonedGroup, 16);
		expect(clonedGroup).to_equal(uint8_t{16});
		choke_group::assignNormalized(clonedGroup, 17);
		expect(clonedGroup).to_equal(uint8_t{1});
	});

	it("writes only a normalized non-default Sound Drum group", _ {
		int32_t writes = 0;
		uint8_t writtenGroup = 0;
		auto record = [&](uint8_t group) {
			++writes;
			writtenGroup = group;
		};

		choke_group::writeIfNonDefault(1, record);
		expect(writes).to_equal(0);
		choke_group::writeIfNonDefault(2, record);
		expect(writes).to_equal(1);
		expect(writtenGroup).to_equal(uint8_t{2});
		choke_group::writeIfNonDefault(17, record);
		expect(writes).to_equal(1);
	});

	it("matches only CHOKE participants in the same normalized group", _ {
		expect(choke_group::matches(PolyphonyMode::CHOKE, 1, 1)).to_be_true();
		expect(choke_group::matches(PolyphonyMode::CHOKE, 2, 2)).to_be_true();
		expect(choke_group::matches(PolyphonyMode::CHOKE, 2, 1)).to_be_false();
		expect(choke_group::matches(PolyphonyMode::POLY, 1, 1)).to_be_false();
		expect(choke_group::matches(PolyphonyMode::MONO, 1, 1)).to_be_false();
	});

	it("releases immediately before a resolved start and does nothing for a no-output arp input", _ {
		ArpReturnInstruction instruction;
		std::string events;
		bool started = choke_group::performImmediateStart(
		    choke_group::hasResolvedImmediateStart(instruction), [&]() { events += "release "; },
		    [&]() { events += "start"; });
		expect(started).to_be_false();
		expect(std::string_view{events}).to_equal(std::string_view{});

		ArpNote resolvedNote;
		resolvedNote.noteCodeOnPostArp.fill(ARP_NOTE_NONE);
		instruction.arpNoteOn = &resolvedNote;
		expect(choke_group::hasResolvedImmediateStart(instruction)).to_be_false();
		resolvedNote.noteCodeOnPostArp[0] = 4;
		started = choke_group::performImmediateStart(
		    choke_group::hasResolvedImmediateStart(instruction), [&]() { events += "release "; },
		    [&]() { events += "start"; });
		expect(started).to_be_true();
		expect(std::string_view{events}).to_equal(std::string_view{"release start"});
	});

	it("keeps schema two for an inactive non-CHOKE group and when combined with External Step", _ {
		compatibility::LocalSaveSchemaRequirement requirement;
		expect(choke_group::participates(PolyphonyMode::POLY)).to_be_false();
		requirement.includeChokeGroup(2);
		expect(requirement.value()).to_equal(compatibility::LocalSaveSchema::CHOKE_GROUPS);

		requirement.includeExternalStep(true);
		requirement.includeChokeGroup(1);
		expect(requirement.value()).to_equal(compatibility::LocalSaveSchema::CHOKE_GROUPS);
	});

	it("writes schema-two Kit and saved-row root markers and rejects a future root", _ {
		compatibility::LocalSaveSchemaRequirement kitRequirement;
		kitRequirement.includeChokeGroup(2);
		CompatibilityMarkerWriter kitWriter;
		compatibility::writeCompatibilityMarker(kitWriter, kitRequirement.value());
		expect(std::string_view{kitWriter.marker}).to_equal(std::string_view{"nl-save-schema-2"});

		CompatibilityMarkerWriter savedRowWriter;
		compatibility::writeCompatibilityMarker(savedRowWriter, compatibility::requiredSchemaForChokeGroup(2));
		expect(std::string_view{savedRowWriter.marker}).to_equal(std::string_view{"nl-save-schema-2"});

		char supportedRoot[] = R"({"firmwareVersion":"c1.3.1","earliestCompatibleFirmware":"nl-save-schema-2","chokeGroup":2})";
		JsonDeserializer supportedReader{reinterpret_cast<uint8_t*>(supportedRoot), sizeof(supportedRoot) - 1};
		expect(deluge::song_load::readJsonRootFirmwareCompatibility(supportedReader)).to_equal(Error::NONE);

		char futureRoot[] = R"({"firmwareVersion":"c1.3.1","earliestCompatibleFirmware":"nl-save-schema-3","chokeGroup":2})";
		JsonDeserializer futureReader{reinterpret_cast<uint8_t*>(futureRoot), sizeof(futureRoot) - 1};
		expect(deluge::song_load::readJsonRootFirmwareCompatibility(futureReader))
		    .to_equal(Error::FILE_FIRMWARE_VERSION_TOO_NEW);
	});

	it("routes contextual Select and resolves exact OLED and seven-segment labels", _ {
		using polyphony_policy::DetailMenu;
		expect(polyphony_policy::detailMenuFor(PolyphonyMode::POLY, false)).to_equal(DetailMenu::VOICE_COUNT);
		expect(polyphony_policy::detailMenuFor(PolyphonyMode::CHOKE, true)).to_equal(DetailMenu::CHOKE_GROUP);
		expect(polyphony_policy::detailMenuFor(PolyphonyMode::CHOKE, false)).to_equal(DetailMenu::NONE);
		expect(polyphony_policy::detailMenuFor(PolyphonyMode::MONO, true)).to_equal(DetailMenu::NONE);

		expect(deluge::l10n::getView(deluge::l10n::built_in::english, polyphony_policy::chokeGroupTitle(true)))
		    .to_equal(std::string_view{"Choke group"});
		expect(deluge::l10n::getView(deluge::l10n::built_in::seven_segment,
		                             polyphony_policy::chokeGroupTitle(false)))
		    .to_equal(std::string_view{"CHGP"});
	});

	it("formats every choke-group value as G01 through G16", _ {
		constexpr std::array expected = {
		    std::string_view{"G01"}, std::string_view{"G02"}, std::string_view{"G03"}, std::string_view{"G04"},
		    std::string_view{"G05"}, std::string_view{"G06"}, std::string_view{"G07"}, std::string_view{"G08"},
		    std::string_view{"G09"}, std::string_view{"G10"}, std::string_view{"G11"}, std::string_view{"G12"},
		    std::string_view{"G13"}, std::string_view{"G14"}, std::string_view{"G15"}, std::string_view{"G16"},
		};
		for (uint8_t group = 1; group <= expected.size(); ++group) {
			char text[4];
			choke_group::formatSevenSegmentValue(group, text);
			expect(std::string_view{text}).to_equal(expected[group - 1]);
		}
	});
});
// clang-format on

CPPSPEC_SPEC(kit_choke_group)
