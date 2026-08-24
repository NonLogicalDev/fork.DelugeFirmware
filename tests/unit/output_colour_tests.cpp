#include "CppUTest/TestHarness.h"

#include "model/output_colour.h"

TEST_GROUP(OutputColourTests){};

TEST(OutputColourTests, stepsWithinTheVisibleHueRange) {
	LONGS_EQUAL(24, deluge::output_colour::step(23, 1));
	LONGS_EQUAL(22, deluge::output_colour::step(23, -1));
}

TEST(OutputColourTests, wrapsWithoutReturningTheUnsetSentinel) {
	LONGS_EQUAL(1, deluge::output_colour::step(191, 1));
	LONGS_EQUAL(191, deluge::output_colour::step(1, -1));
}

TEST(OutputColourTests, coarseStepsAlsoSkipTheUnsetSentinel) {
	LONGS_EQUAL(1, deluge::output_colour::step(170, 22));
	LONGS_EQUAL(191, deluge::output_colour::step(23, -23));
}

TEST(OutputColourTests, normalizesAcceleratedSteps) {
	LONGS_EQUAL(19, deluge::output_colour::step(191, 20));
	LONGS_EQUAL(173, deluge::output_colour::step(1, -20));
}

TEST(OutputColourTests, audioClipStepDoesNotDependOnEncoderBatching) {
	int32_t twoSingleSteps =
	    deluge::output_colour::audioClipHueOffset(1) + deluge::output_colour::audioClipHueOffset(1);
	LONGS_EQUAL(twoSingleSteps, deluge::output_colour::audioClipHueOffset(2));
	LONGS_EQUAL(-6, twoSingleSteps);
}
