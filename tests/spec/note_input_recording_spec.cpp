#include "gui/note_input_recording.h"

#include "cppspec.hpp"

using deluge::gui::note_input::RecordingLifecycle;
using deluge::gui::note_input::RecordingTransition;

// clang-format off
describe note_input_recording("Note-input recording lifecycle", $ {
	it("keeps preview-only ownership after Horizontal is released", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(3, true)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.isActive(3)).to_be_true();
		expect(lifecycle.isPreviewOnly(3)).to_be_true();

		lifecycle.noteOnRecordingResult(3, true);
		expect(lifecycle.hasRecordedNoteOn(3)).to_be_false();
		expect(lifecycle.noteOff(3)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.isActive(3)).to_be_false();
	});

	it("retains an ordinary recorded note-off after Horizontal is pressed", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(4, false)).to_equal(RecordingTransition::NOTE_ON);
		lifecycle.noteOnRecordingResult(4, true);
		expect(lifecycle.hasRecordedNoteOn(4)).to_be_true();

		expect(lifecycle.noteOff(4)).to_equal(RecordingTransition::NOTE_OFF);
		expect(lifecycle.noteOff(4)).to_equal(RecordingTransition::SUPPRESS);
	});

	it("keeps a preview row owned across Note Row Editor re-audition", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(3, true)).to_equal(RecordingTransition::SUPPRESS);

		// Note Row Editor stops and silently re-auditions the row without changing
		// the physical pad gesture's recording lifecycle.
		expect(lifecycle.isActive(3)).to_be_true();
		expect(lifecycle.isPreviewOnly(3)).to_be_true();
		expect(lifecycle.hasRecordedNoteOn(3)).to_be_false();

		// Releasing Horizontal before leaving the editor must not turn the final
		// synthetic release into a recorded note-off.
		expect(lifecycle.noteOff(3)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.noteOff(3)).to_equal(RecordingTransition::SUPPRESS);
	});

	it("keeps an ordinary recorded row paired across Note Row Editor re-audition", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(4, false)).to_equal(RecordingTransition::NOTE_ON);
		lifecycle.noteOnRecordingResult(4, true);

		// Note Row Editor's synthetic stop and silent re-audition leave the
		// physical note-on ownership intact until the editor closes it.
		expect(lifecycle.isActive(4)).to_be_true();
		expect(lifecycle.isPreviewOnly(4)).to_be_false();
		expect(lifecycle.hasRecordedNoteOn(4)).to_be_true();

		expect(lifecycle.noteOff(4)).to_equal(RecordingTransition::NOTE_OFF);
		expect(lifecycle.noteOff(4)).to_equal(RecordingTransition::SUPPRESS);
	});

	it("does not emit a note-off when the ordinary note-on was not recorded", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(2, false)).to_equal(RecordingTransition::NOTE_ON);
		lifecycle.noteOnRecordingResult(2, false);
		expect(lifecycle.noteOff(2)).to_equal(RecordingTransition::SUPPRESS);
	});

	it("handles independent notes and retriggers without leaking ownership", _ {
		RecordingLifecycle<8> lifecycle;
		expect(lifecycle.noteOn(1, false)).to_equal(RecordingTransition::NOTE_ON);
		lifecycle.noteOnRecordingResult(1, true);
		expect(lifecycle.noteOn(5, true)).to_equal(RecordingTransition::SUPPRESS);

		expect(lifecycle.noteOff(1)).to_equal(RecordingTransition::NOTE_OFF);
		expect(lifecycle.noteOff(5)).to_equal(RecordingTransition::SUPPRESS);

		expect(lifecycle.noteOn(1, true)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.noteOff(1)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.noteOn(1, false)).to_equal(RecordingTransition::NOTE_ON);
		lifecycle.noteOnRecordingResult(1, true);
		expect(lifecycle.noteOff(1)).to_equal(RecordingTransition::NOTE_OFF);
	});

	it("clears stale ownership during view cleanup", _ {
		RecordingLifecycle<8> lifecycle;
		lifecycle.noteOn(0, false);
		lifecycle.noteOnRecordingResult(0, true);
		lifecycle.noteOn(7, true);
		lifecycle.clear();

		expect(lifecycle.isActive(0)).to_be_false();
		expect(lifecycle.isActive(7)).to_be_false();
		expect(lifecycle.noteOff(0)).to_equal(RecordingTransition::SUPPRESS);
		expect(lifecycle.noteOff(7)).to_equal(RecordingTransition::SUPPRESS);
	});
});
// clang-format on

CPPSPEC_SPEC(note_input_recording)
