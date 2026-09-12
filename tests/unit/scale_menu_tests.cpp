#include "CppUTest/TestHarness.h"
#include "gui/ui/scale_menu_policy.h"

TEST_GROUP(ScaleMenu){};

TEST(ScaleMenu, ExcludedPressAndReleasePassThrough) {
	ScaleMenuGesture gesture;
	CHECK(gesture.handle(true, false, false) == ScaleMenuGesture::Result::PASS);
	CHECK(gesture.handle(false, true, false) == ScaleMenuGesture::Result::PASS);
}

TEST(ScaleMenu, OwnsReleaseAfterShiftReleaseOrMenuClosure) {
	ScaleMenuGesture gesture;
	CHECK(gesture.handle(true, true, false) == ScaleMenuGesture::Result::OPEN);
	// Eligibility is now false because Shift was released or the originating view changed.
	CHECK(gesture.handle(false, false, false) == ScaleMenuGesture::Result::CONSUME);
	CHECK(gesture.handle(false, false, false) == ScaleMenuGesture::Result::PASS);
	CHECK(gesture.handle(true, true, false) == ScaleMenuGesture::Result::OPEN);
}

TEST(ScaleMenu, RepeatedDownDoesNotReopenMenu) {
	ScaleMenuGesture gesture;
	CHECK(gesture.handle(true, true, false) == ScaleMenuGesture::Result::OPEN);
	CHECK(gesture.handle(true, true, false) == ScaleMenuGesture::Result::CONSUME);
	CHECK(gesture.handle(false, true, false) == ScaleMenuGesture::Result::CONSUME);
}

TEST(ScaleMenu, CardRoutineDefersBeforeTakingOwnership) {
	ScaleMenuGesture gesture;
	CHECK(gesture.handle(true, true, true) == ScaleMenuGesture::Result::DEFER);
	CHECK(gesture.handle(true, true, false) == ScaleMenuGesture::Result::OPEN);
	CHECK(gesture.handle(false, false, true) == ScaleMenuGesture::Result::CONSUME);
}

TEST(ScaleMenu, UnavailableModesAreSkippedInBothDirections) {
	std::bitset<NUM_ALL_SCALES> available;
	available[MAJOR_SCALE] = true;
	available[DORIAN_SCALE] = true;
	available[USER_SCALE] = true;
	CHECK_EQUAL(DORIAN_SCALE, stepScaleMenuMode(MAJOR_SCALE, 1, available));
	CHECK_EQUAL(USER_SCALE, stepScaleMenuMode(MAJOR_SCALE, -1, available));
	CHECK_EQUAL(MAJOR_SCALE, stepScaleMenuMode(USER_SCALE, 1, available));
	CHECK_EQUAL(USER_SCALE, stepScaleMenuMode(MAJOR_SCALE, 2, available));
}

TEST(ScaleMenu, NoAvailableModeAndZeroMovementPreserveCurrentState) {
	std::bitset<NUM_ALL_SCALES> available;
	CHECK_EQUAL(USER_SCALE, stepScaleMenuMode(USER_SCALE, 10, available));
	available[MAJOR_SCALE] = true;
	CHECK_EQUAL(USER_SCALE, stepScaleMenuMode(USER_SCALE, 0, available));
	CHECK_EQUAL(MAJOR_SCALE, stepScaleMenuMode(MAJOR_SCALE, -128, available));
}

TEST(ScaleMenu, UnlistedCustomModeEntersAvailableListPredictably) {
	std::bitset<NUM_ALL_SCALES> available;
	available[MAJOR_SCALE] = true;
	available[DORIAN_SCALE] = true;
	CHECK_EQUAL(MAJOR_SCALE, stepScaleMenuMode(NO_SCALE, 1, available));
	CHECK_EQUAL(DORIAN_SCALE, stepScaleMenuMode(NO_SCALE, -1, available));
}
