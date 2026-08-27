#include "io/midi/external_step_midi_input.h"

#include "cppspec.hpp"

#include <cstdint>

class MIDICable {};

using deluge::midi::ExternalStepMIDIInput;
using deluge::midi::ExternalStepMIDIMessageType;

namespace {

ExternalStepMIDIInput assignedInput(MIDICable& cable, ExternalStepMIDIMessageType type, uint8_t number) {
	ExternalStepMIDIInput input;
	input.cable = &cable;
	input.channel = 4;
	input.messageType = type;
	input.number = number;
	return input;
}

} // namespace

// clang-format off
describe external_step_midi_input("External Step MIDI input", $ {
	it("requires a complete exact assignment", _ {
		MIDICable cableA;
		MIDICable cableB;
		auto input = assignedInput(cableA, ExternalStepMIDIMessageType::NOTE, 60);

		expect(input.isAssigned()).to_be_true();
		expect(input.matches(cableA, 4, ExternalStepMIDIMessageType::NOTE, 60)).to_be_true();
		expect(input.matches(cableB, 4, ExternalStepMIDIMessageType::NOTE, 60)).to_be_false();
		expect(input.matches(cableA, 5, ExternalStepMIDIMessageType::NOTE, 60)).to_be_false();
		expect(input.matches(cableA, 4, ExternalStepMIDIMessageType::CC, 60)).to_be_false();
		expect(input.matches(cableA, 4, ExternalStepMIDIMessageType::NOTE, 61)).to_be_false();

		input.clear();
		expect(input.isAssigned()).to_be_false();
		expect(input.matches(cableA, 4, ExternalStepMIDIMessageType::NOTE, 60)).to_be_false();
	});

	it("identifies only the same complete assignment", _ {
		MIDICable cableA;
		MIDICable cableB;
		auto input = assignedInput(cableA, ExternalStepMIDIMessageType::CC, 16);
		auto identical = assignedInput(cableA, ExternalStepMIDIMessageType::CC, 16);
		expect(input.hasSameAssignmentAs(identical)).to_be_true();

		for (auto distinct : {
		         assignedInput(cableB, ExternalStepMIDIMessageType::CC, 16),
		         assignedInput(cableA, ExternalStepMIDIMessageType::NOTE, 16),
		         assignedInput(cableA, ExternalStepMIDIMessageType::CC, 17),
		     }) {
			expect(input.hasSameAssignmentAs(distinct)).to_be_false();
		}
		identical.channel = 5;
		expect(input.hasSameAssignmentAs(identical)).to_be_false();

		ExternalStepMIDIInput unassigned;
		expect(unassigned.hasSameAssignmentAs(unassigned)).to_be_false();
	});

	it("validates the physical channel and each message-number domain", _ {
		MIDICable cable;
		auto input = assignedInput(cable, ExternalStepMIDIMessageType::NOTE, 127);
		expect(input.isAssigned()).to_be_true();

		input.channel = 16;
		expect(input.isAssigned()).to_be_false();
		input.channel = 4;

		input.messageType = ExternalStepMIDIMessageType::CC;
		input.number = 119;
		expect(input.isAssigned()).to_be_true();
		input.number = 120;
		expect(input.isAssigned()).to_be_false();

		input.messageType = ExternalStepMIDIMessageType::PROGRAM_CHANGE;
		input.number = 127;
		expect(input.isAssigned()).to_be_true();
		input.messageType = ExternalStepMIDIMessageType::NONE;
		expect(input.isAssigned()).to_be_false();
	});

	it("reports one Note edge until release rearms it", _ {
		MIDICable cable;
		auto input = assignedInput(cable, ExternalStepMIDIMessageType::NOTE, 64);

		expect(input.observeMessage(true)).to_be_true();
		expect(input.observeMessage(true)).to_be_false();
		expect(input.observeMessage(false)).to_be_false();
		expect(input.observeMessage(true)).to_be_true();
	});

	it("reports one CC edge per low-to-high threshold crossing", _ {
		MIDICable cable;
		auto input = assignedInput(cable, ExternalStepMIDIMessageType::CC, 16);

		expect(input.observeMessage(false)).to_be_false();
		expect(input.observeMessage(true)).to_be_true();
		expect(input.observeMessage(true)).to_be_false();
		expect(input.observeMessage(false)).to_be_false();
		expect(input.observeMessage(true)).to_be_true();
	});

	it("treats every matching Program Change as an edge", _ {
		MIDICable cable;
		auto input = assignedInput(cable, ExternalStepMIDIMessageType::PROGRAM_CHANGE, 12);

		expect(input.observeMessage(true)).to_be_true();
		expect(input.observeMessage(true)).to_be_true();
		expect(input.observeMessage(false)).to_be_true();
	});

	it("does not copy transient held or conflict state", _ {
		MIDICable cable;
		auto source = assignedInput(cable, ExternalStepMIDIMessageType::NOTE, 60);
		expect(source.observeMessage(true)).to_be_true();
		source.conflict = true;

		ExternalStepMIDIInput copy = source;
		expect(copy.isAssigned()).to_be_true();
		expect(copy.high).to_be_false();
		expect(copy.conflict).to_be_false();
		expect(copy.observeMessage(true)).to_be_true();

		ExternalStepMIDIInput assigned;
		assigned = source;
		expect(assigned.high).to_be_false();
		expect(assigned.conflict).to_be_false();
	});

	it("clears a held edge without erasing its assignment or conflict", _ {
		MIDICable cable;
		auto input = assignedInput(cable, ExternalStepMIDIMessageType::NOTE, 60);
		expect(input.observeMessage(true)).to_be_true();
		input.conflict = true;

		input.clearHeldState();
		expect(input.isAssigned()).to_be_true();
		expect(input.high).to_be_false();
		expect(input.conflict).to_be_true();
		expect(input.observeMessage(true)).to_be_true();
	});

	it("rearms both configured controls for Stop without erasing their assignments", _ {
		MIDICable cable;
		auto step = assignedInput(cable, ExternalStepMIDIMessageType::NOTE, 60);
		auto reset = assignedInput(cable, ExternalStepMIDIMessageType::CC, 16);
		expect(step.observeMessage(true)).to_be_true();
		expect(reset.observeMessage(true)).to_be_true();

		step.clearHeldState();
		reset.clearHeldState();
		expect(step.isAssigned()).to_be_true();
		expect(reset.isAssigned()).to_be_true();
		expect(step.observeMessage(true)).to_be_true();
		expect(reset.observeMessage(true)).to_be_true();
	});

	it("rearms held Note and CC controls only when conflict state changes", _ {
		MIDICable cable;
		for (auto type : {ExternalStepMIDIMessageType::NOTE, ExternalStepMIDIMessageType::CC}) {
			auto input = assignedInput(cable, type, 16);
			expect(input.observeMessage(true)).to_be_true();
			expect(input.updateConflictState(true)).to_be_true();
			expect(input.high).to_be_false();
			expect(input.observeMessage(true)).to_be_true();
			expect(input.updateConflictState(true)).to_be_false();
			expect(input.high).to_be_true();
			expect(input.updateConflictState(false)).to_be_true();
			expect(input.high).to_be_false();
			expect(input.observeMessage(true)).to_be_true();
		}

		auto program = assignedInput(cable, ExternalStepMIDIMessageType::PROGRAM_CHANGE, 12);
		expect(program.updateConflictState(true)).to_be_true();
		expect(program.observeMessage(true)).to_be_true();
		expect(program.updateConflictState(false)).to_be_true();
		expect(program.observeMessage(true)).to_be_true();
	});
});
// clang-format on

CPPSPEC_SPEC(external_step_midi_input)
