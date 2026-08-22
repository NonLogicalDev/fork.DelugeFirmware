#include "model/instrument/midi_sustain_deferred_notes.h"

#include "cppspec.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

using deluge::midi_support::SustainDeferredNotes;

static_assert(sizeof(SustainDeferredNotes) == 128);
static_assert(std::is_trivially_copyable_v<SustainDeferredNotes>);

// clang-format off
describe midi_sustain_deferred_notes("MIDI sustain deferred note state", $ {
	it("defers and releases every MIDI pitch with its exact release velocity", _ {
		SustainDeferredNotes notes;
		std::array<bool, SustainDeferredNotes::kNoteCount> released{};
		int32_t releaseCount = 0;

		for (int32_t note = 0; note < SustainDeferredNotes::kNoteCount; ++note) {
			expect(notes.defer(note, static_cast<uint8_t>(127 - note))).to_be_true();
			expect(notes.contains(note)).to_be_true();
		}
		expect(notes.empty()).to_be_false();

		notes.releaseAll([&](int32_t note, uint8_t velocity) {
			expect(released[note]).to_be_false();
			expect(velocity).to_equal(static_cast<uint8_t>(127 - note));
			released[note] = true;
			releaseCount++;
		});

		expect(releaseCount).to_equal(SustainDeferredNotes::kNoteCount);
		expect(notes.empty()).to_be_true();
		for (bool wasReleased : released) {
			expect(wasReleased).to_be_true();
		}
	});

	it("replaces velocity for repeated note-offs and cancels release on retrigger", _ {
		SustainDeferredNotes notes;
		expect(notes.defer(64, 0)).to_be_true();
		expect(notes.defer(64, 127)).to_be_true();

		int32_t releaseCount = 0;
		notes.releaseAll([&](int32_t note, uint8_t velocity) {
			expect(note).to_equal(64);
			expect(velocity).to_equal(uint8_t{127});
			releaseCount++;
		});
		expect(releaseCount).to_equal(1);

		expect(notes.defer(64, 45)).to_be_true();
		notes.erase(64);
		notes.releaseAll([&](int32_t, uint8_t) { releaseCount++; });
		expect(releaseCount).to_equal(1);
		expect(notes.empty()).to_be_true();
	});

	it("clears all deferred notes without releasing them", _ {
		SustainDeferredNotes notes;
		expect(notes.defer(0, 1)).to_be_true();
		expect(notes.defer(127, 2)).to_be_true();
		notes.clear();

		int32_t releaseCount = 0;
		notes.releaseAll([&](int32_t, uint8_t) { releaseCount++; });
		expect(releaseCount).to_equal(0);
		expect(notes.empty()).to_be_true();
	});

	it("rejects pitches outside the fixed MIDI domain", _ {
		SustainDeferredNotes notes;
		expect(notes.defer(-1, 64)).to_be_false();
		expect(notes.defer(SustainDeferredNotes::kNoteCount, 64)).to_be_false();
		expect(notes.contains(-1)).to_be_false();
		expect(notes.contains(SustainDeferredNotes::kNoteCount)).to_be_false();
		notes.erase(-1);
		notes.erase(SustainDeferredNotes::kNoteCount);
		expect(notes.empty()).to_be_true();
	});
});
// clang-format on

CPPSPEC_SPEC(midi_sustain_deferred_notes)
