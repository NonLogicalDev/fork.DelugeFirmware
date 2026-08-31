#include "CppUTest/TestHarness.h"

#include "gui/ui/slicer_playhead.h"

using deluge::gui::slicer_playhead::Source;

TEST_GROUP(SlicerPlayheadTests){};

TEST(SlicerPlayheadTests, regionInNewKitFollowsBrowserPreview) {
	CHECK_EQUAL(static_cast<int32_t>(Source::BROWSER_PREVIEW),
	            static_cast<int32_t>(deluge::gui::slicer_playhead::getSource(true, false)));
}

TEST(SlicerPlayheadTests, manualAndExistingKitFollowSelectedSound) {
	CHECK_EQUAL(static_cast<int32_t>(Source::SELECTED_SOUND),
	            static_cast<int32_t>(deluge::gui::slicer_playhead::getSource(false, false)));
	CHECK_EQUAL(static_cast<int32_t>(Source::SELECTED_SOUND),
	            static_cast<int32_t>(deluge::gui::slicer_playhead::getSource(true, true)));
	CHECK_EQUAL(static_cast<int32_t>(Source::SELECTED_SOUND),
	            static_cast<int32_t>(deluge::gui::slicer_playhead::getSource(false, true)));
}

TEST(SlicerPlayheadTests, regionUsesEveryRowAndManualUsesOnlyWaveformRows) {
	for (int32_t row = 0; row < kDisplayHeight; row++) {
		CHECK_TRUE(deluge::gui::slicer_playhead::isWaveformRow(true, row));
		CHECK_EQUAL(row >= kDisplayHeight / 2, deluge::gui::slicer_playhead::isWaveformRow(false, row));
	}
}
