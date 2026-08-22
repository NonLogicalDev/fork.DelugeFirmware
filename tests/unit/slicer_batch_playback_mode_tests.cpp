#include "CppUTest/TestHarness.h"

#include "gui/ui/slicer_batch_playback_mode.h"

using deluge::gui::slicer_playback::BatchMode;

TEST_GROUP(SlicerBatchPlaybackModeTests){};

TEST(SlicerBatchPlaybackModeTests, mapsEachMenuIndexToItsBatchMode) {
	BatchMode mode = BatchMode::ONCE;

	CHECK_TRUE(deluge::gui::slicer_playback::setFromMenuIndex(mode, 0));
	LONGS_EQUAL(static_cast<int32_t>(BatchMode::AUTO), static_cast<int32_t>(mode));
	CHECK_TRUE(deluge::gui::slicer_playback::setFromMenuIndex(mode, 1));
	LONGS_EQUAL(static_cast<int32_t>(BatchMode::CUT), static_cast<int32_t>(mode));
	CHECK_TRUE(deluge::gui::slicer_playback::setFromMenuIndex(mode, 2));
	LONGS_EQUAL(static_cast<int32_t>(BatchMode::ONCE), static_cast<int32_t>(mode));
}

TEST(SlicerBatchPlaybackModeTests, rejectsInvalidMenuIndexWithoutChangingMode) {
	BatchMode mode = BatchMode::CUT;

	CHECK_FALSE(deluge::gui::slicer_playback::setFromMenuIndex(mode, -1));
	LONGS_EQUAL(static_cast<int32_t>(BatchMode::CUT), static_cast<int32_t>(mode));
	CHECK_FALSE(deluge::gui::slicer_playback::setFromMenuIndex(mode, 3));
	LONGS_EQUAL(static_cast<int32_t>(BatchMode::CUT), static_cast<int32_t>(mode));
}

TEST(SlicerBatchPlaybackModeTests, mapsEachBatchModeBackToItsMenuIndex) {
	LONGS_EQUAL(0, deluge::gui::slicer_playback::toMenuIndex(BatchMode::AUTO));
	LONGS_EQUAL(1, deluge::gui::slicer_playback::toMenuIndex(BatchMode::CUT));
	LONGS_EQUAL(2, deluge::gui::slicer_playback::toMenuIndex(BatchMode::ONCE));
}

TEST(SlicerBatchPlaybackModeTests, autoUsesOnceForShortSlicesAndConfiguredModeOtherwise) {
	LONGS_EQUAL(
	    static_cast<int32_t>(SampleRepeatMode::ONCE),
	    static_cast<int32_t>(deluge::gui::slicer_playback::resolve(BatchMode::AUTO, 2001, SampleRepeatMode::CUT)));
	LONGS_EQUAL(static_cast<int32_t>(SampleRepeatMode::CUT), static_cast<int32_t>(deluge::gui::slicer_playback::resolve(
	                                                             BatchMode::AUTO, 2002, SampleRepeatMode::CUT)));
	LONGS_EQUAL(
	    static_cast<int32_t>(SampleRepeatMode::LOOP),
	    static_cast<int32_t>(deluge::gui::slicer_playback::resolve(BatchMode::AUTO, 8000, SampleRepeatMode::LOOP)));
}

TEST(SlicerBatchPlaybackModeTests, cutOverridesDurationAndConfiguredMode) {
	LONGS_EQUAL(static_cast<int32_t>(SampleRepeatMode::CUT),
	            static_cast<int32_t>(deluge::gui::slicer_playback::resolve(BatchMode::CUT, 1, SampleRepeatMode::ONCE)));
	LONGS_EQUAL(static_cast<int32_t>(SampleRepeatMode::CUT), static_cast<int32_t>(deluge::gui::slicer_playback::resolve(
	                                                             BatchMode::CUT, 8000, SampleRepeatMode::LOOP)));
}

TEST(SlicerBatchPlaybackModeTests, onceOverridesDurationAndConfiguredMode) {
	LONGS_EQUAL(static_cast<int32_t>(SampleRepeatMode::ONCE),
	            static_cast<int32_t>(deluge::gui::slicer_playback::resolve(BatchMode::ONCE, 1, SampleRepeatMode::CUT)));
	LONGS_EQUAL(
	    static_cast<int32_t>(SampleRepeatMode::ONCE),
	    static_cast<int32_t>(deluge::gui::slicer_playback::resolve(BatchMode::ONCE, 8000, SampleRepeatMode::LOOP)));
}
