#include "gui/menu_item/midi/external_step_policy.h"
#include "model/song/project_compatibility.h"

#include "cppspec.hpp"

namespace external_step_policy = deluge::gui::menu_item::midi::external_step;
using deluge::external_step::Status;
using deluge::gui::menu_item::midi::external_step::StatusLabel;
using deluge::midi::ExternalStepMIDIInput;
using deluge::midi::ExternalStepMIDIMessageType;

namespace {

ExternalStepMIDIInput assignedInput(MIDICable* cable, uint8_t channel, ExternalStepMIDIMessageType type,
                                    uint8_t number) {
	ExternalStepMIDIInput input;
	input.cable = cable;
	input.channel = channel;
	input.messageType = type;
	input.number = number;
	return input;
}

} // namespace

// clang-format off
describe external_step_menu("External Step menu and compatibility", $ {
	it("shows Clock for enabled MIDI Clips and feature-Off recovery only", _ {
		expect(external_step_policy::shouldShowClockMenu(true, true, false)).to_be_true();
		expect(external_step_policy::shouldShowClockMenu(true, false, true)).to_be_true();
		expect(external_step_policy::shouldShowClockMenu(true, false, false)).to_be_false();
		expect(external_step_policy::shouldShowClockMenu(false, true, true)).to_be_false();
	});

	it("rejects only exact Step and Reset assignment duplication", _ {
		uint8_t cableStorage[2];
		auto* cableA = reinterpret_cast<MIDICable*>(&cableStorage[0]);
		auto* cableB = reinterpret_cast<MIDICable*>(&cableStorage[1]);
		auto first = assignedInput(cableA, 3, ExternalStepMIDIMessageType::NOTE, 60);
		auto identical = assignedInput(cableA, 3, ExternalStepMIDIMessageType::NOTE, 60);
		expect(external_step_policy::assignmentsAreIdentical(first, identical)).to_be_true();

		for (auto distinct : {
		         assignedInput(cableB, 3, ExternalStepMIDIMessageType::NOTE, 60),
		         assignedInput(cableA, 4, ExternalStepMIDIMessageType::NOTE, 60),
		         assignedInput(cableA, 3, ExternalStepMIDIMessageType::CC, 60),
		         assignedInput(cableA, 3, ExternalStepMIDIMessageType::NOTE, 61),
		     }) {
			expect(external_step_policy::assignmentsAreIdentical(first, distinct)).to_be_false();
		}

		ExternalStepMIDIInput unassigned;
		expect(external_step_policy::assignmentsAreIdentical(unassigned, unassigned)).to_be_false();
	});

	it("remembers a learned held Note but rearms learned CC and Program Change controls", _ {
		expect(external_step_policy::shouldRememberHighStateAfterLearn(ExternalStepMIDIMessageType::NOTE)).to_be_true();
		expect(external_step_policy::shouldRememberHighStateAfterLearn(ExternalStepMIDIMessageType::CC)).to_be_false();
		expect(external_step_policy::shouldRememberHighStateAfterLearn(ExternalStepMIDIMessageType::PROGRAM_CHANGE))
		    .to_be_false();
		expect(external_step_policy::shouldRememberHighStateAfterLearn(ExternalStepMIDIMessageType::NONE)).to_be_false();
	});

	it("maps each runtime status to a distinct recovery label", _ {
		expect(external_step_policy::statusLabel(Status::SONG)).to_equal(StatusLabel::SONG);
		expect(external_step_policy::statusLabel(Status::FEATURE_DISABLED)).to_equal(StatusLabel::OFF);
		expect(external_step_policy::statusLabel(Status::UNASSIGNED)).to_equal(StatusLabel::UNASSIGNED);
		expect(external_step_policy::statusLabel(Status::MISSING_INPUT)).to_equal(StatusLabel::MISSING_INPUT);
		expect(external_step_policy::statusLabel(Status::CONFLICT)).to_equal(StatusLabel::CONFLICT);
		expect(external_step_policy::statusLabel(Status::UNSUPPORTED)).to_equal(StatusLabel::UNSUPPORTED);
		expect(external_step_policy::statusLabel(Status::WAIT)).to_equal(StatusLabel::WAITING);
		expect(external_step_policy::statusLabel(Status::READY)).to_equal(StatusLabel::READY);
	});

	it("guards only Songs that still contain External Step Clips", _ {
		using namespace deluge::project_compatibility;
		LocalSaveSchemaRequirement requirement;
		requirement.includeExternalStep(false);
		expect(requirement.value()).to_equal(LocalSaveSchema::NONE);
		expect(earliestCompatibleFirmware(requirement.value())).to_equal(kLegacySongMinimumFirmware);

		requirement.includeExternalStep(true);
		requirement.includeExternalStep(false);
		expect(requirement.value()).to_equal(LocalSaveSchema::EXTERNAL_STEP);
		expect(earliestCompatibleFirmware(requirement.value())).to_equal(kExternalStepMinimumFirmware);
		expect(kExternalStepMinimumFirmware).to_equal(std::string_view{"nl-save-schema-1"});

		requirement.includeChokeGroup(2);
		expect(requirement.value()).to_equal(LocalSaveSchema::CHOKE_GROUPS);
		expect(earliestCompatibleFirmware(requirement.value())).to_equal(kChokeGroupsMinimumFirmware);
	});
});

CPPSPEC_SPEC(external_step_menu)
