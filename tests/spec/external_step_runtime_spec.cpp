#include "model/clip/external_step_runtime.h"

#include "cppspec.hpp"

using deluge::external_step::noteDurationInPulses;
using deluge::external_step::Runtime;
using deluge::external_step::State;

// clang-format off
describe external_step_runtime("External Step Clip runtime", $ {
	it("emits zero first, advances one boundary, and wraps", _ {
		Runtime runtime;
		auto first = runtime.advance(42, 24, 6, 1000);
		expect(first.emit).to_be_true();
		expect(first.position).to_equal(uint32_t{0});
		expect(first.wrapped).to_be_false();

		auto second = runtime.advance(first.position, 24, 6, 3000);
		expect(second.position).to_equal(uint32_t{6});
		auto wrapped = runtime.advance(18, 24, 6, 5000);
		expect(wrapped.position).to_equal(uint32_t{0});
		expect(wrapped.wrapped).to_be_true();
	});

	it("makes Reset pre-zero and WAIT preserve the current phase", _ {
		Runtime runtime;
		static_cast<void>(runtime.advance(0, 24, 6, 1000));
		static_cast<void>(runtime.advance(0, 24, 6, 3000));
		runtime.waitPreservingPhase();
		expect(runtime.state()).to_equal(State::WAIT_CONTINUE);
		auto continued = runtime.advance(6, 24, 6, 5000);
		expect(continued.position).to_equal(uint32_t{12});

		runtime.resetToPreZero();
		expect(runtime.state()).to_equal(State::PRE_ZERO);
		auto resetFirst = runtime.advance(12, 24, 6, 7000);
		expect(resetFirst.position).to_equal(uint32_t{0});
	});

	it("returns Stop cleanup to pre-zero and clears the watchdog", _ {
		Runtime runtime;
		static_cast<void>(runtime.advance(0, 24, 6, 0));
		static_cast<void>(runtime.advance(0, 24, 6, 2000));
		static_cast<void>(runtime.advance(6, 24, 6, 4000));
		static_cast<void>(runtime.advance(12, 24, 6, 6000));
		expect(runtime.hasTimeoutDeadline()).to_be_true();

		runtime.resetToPreZero();
		expect(runtime.state()).to_equal(State::PRE_ZERO);
		expect(runtime.hasTimeoutDeadline()).to_be_false();
		auto firstAfterStop = runtime.advance(18, 24, 6, 8000);
		expect(firstAfterStop.position).to_equal(uint32_t{0});
	});

	it("rejects zero and indivisible timing without changing phase", _ {
		Runtime runtime;
		expect(runtime.advance(0, 25, 6, 1000).emit).to_be_false();
		expect(runtime.advance(0, 24, 0, 1000).emit).to_be_false();
		expect(runtime.state()).to_equal(State::PRE_ZERO);
	});

	it("arms after three stable intervals and expires after four missed intervals", _ {
		Runtime runtime;
		static_cast<void>(runtime.advance(0, 24, 6, 0));
		static_cast<void>(runtime.advance(0, 24, 6, 2000));
		static_cast<void>(runtime.advance(6, 24, 6, 4000));
		expect(runtime.hasTimeoutDeadline()).to_be_false();
		static_cast<void>(runtime.advance(12, 24, 6, 6000));
		expect(runtime.hasTimeoutDeadline()).to_be_true();
		expect(runtime.timeoutDeadline()).to_equal(uint32_t{14000});
		expect(runtime.timeoutDue(13999)).to_be_false();
		expect(runtime.timeoutDue(14000)).to_be_true();
	});

	it("does not arm for unstable or out-of-range cadence", _ {
		Runtime unstable;
		static_cast<void>(unstable.advance(0, 24, 6, 0));
		static_cast<void>(unstable.advance(0, 24, 6, 2000));
		static_cast<void>(unstable.advance(6, 24, 6, 4000));
		static_cast<void>(unstable.advance(12, 24, 6, 8000));
		expect(unstable.hasTimeoutDeadline()).to_be_false();

		Runtime tooSlow;
		static_cast<void>(tooSlow.advance(0, 24, 6, 0));
		static_cast<void>(tooSlow.advance(0, 24, 6, 200000));
		static_cast<void>(tooSlow.advance(6, 24, 6, 400000));
		static_cast<void>(tooSlow.advance(12, 24, 6, 600000));
		expect(tooSlow.hasTimeoutDeadline()).to_be_false();
	});

	it("quantizes note ends to emitted pulses with a one-pulse minimum", _ {
		expect(noteDurationInPulses(0, 1, 0, 96, 6, false)).to_equal(uint32_t{1});
		expect(noteDurationInPulses(1, 1, 6, 96, 6, false)).to_equal(uint32_t{1});
		expect(noteDurationInPulses(1, 12, 6, 96, 6, false)).to_equal(uint32_t{2});
		expect(noteDurationInPulses(90, 12, 0, 96, 6, true)).to_equal(uint32_t{1});
		expect(noteDurationInPulses(95, 10, 0, 96, 6, true)).to_equal(uint32_t{2});
	});
});
// clang-format on

CPPSPEC_SPEC(external_step_runtime)
