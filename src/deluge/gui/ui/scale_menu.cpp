#include "gui/ui/scale_menu.h"
#include "extern.h"

#include "gui/ui/keyboard/keyboard_screen.h"
#include "gui/ui/root_ui.h"
#include "gui/views/arranger_view.h"
#include "gui/views/automation_view.h"
#include "gui/views/instrument_clip_view.h"
#include "gui/views/performance_view.h"
#include "gui/views/session_view.h"
#include "hid/buttons.h"
#include "hid/display/display.h"
#include "hid/led/indicator_leds.h"
#include "hid/matrix/matrix_driver.h"
#include "io/midi/device_specific/specific_midi_device.h"
#include "model/action/action_logger.h"
#include "model/clip/clip.h"
#include "model/output.h"
#include "model/song/song.h"
#include <algorithm>

ScaleMenu scaleMenu;

namespace {
constexpr const char* rootNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
int32_t pitchClass(int32_t note) {
	return (note % 12 + 12) % 12;
}
} // namespace

bool ScaleMenu::canOpen(bool otherButtonHeld) {
	UI* ui = getCurrentUI();
	if (!currentSong || ui != getRootUI() || currentUIMode != UI_MODE_NONE || otherButtonHeld
	    || !Buttons::isShiftButtonPressed()) {
		return false;
	}
	for (int32_t x = 0; x < kDisplayWidth + kSideBarWidth; ++x) {
		for (int32_t y = 0; y < kDisplayHeight; ++y) {
			if (matrixDriver.isPadPressed(x, y)) {
				return false;
			}
		}
	}
	if (ui == &sessionView || ui == &arrangerView || ui == &performanceView
	    || (ui == &automationView && automationView.onArrangerView)) {
		return true;
	}
	if (ui != &instrumentClipView && ui != &keyboardScreen && ui != &automationView) {
		return false;
	}
	Clip* clip = getCurrentClip();
	return clip && clip->type == ClipType::INSTRUMENT && clip->output && clip->output->type != OutputType::KIT;
}

ActionResult ScaleMenu::handleScaleButton(bool on, bool inCardRoutine, bool otherButtonHeld) {
	using Result = ScaleMenuGesture::Result;
	switch (gesture_.handle(on, canOpen(otherButtonHeld), inCardRoutine)) {
	case Result::PASS:
		return ActionResult::NOT_DEALT_WITH;
	case Result::DEFER:
		return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
	case Result::OPEN:
		// Do not replay the gesture into a different UI if opening fails.
		openUI(this);
		[[fallthrough]];
	case Result::CONSUME:
		return ActionResult::DEALT_WITH;
	}
	return ActionResult::NOT_DEALT_WITH;
}

bool ScaleMenu::opened() {
	// Scale is handled before the native Clip view sees it; dismiss its Shift shortcut colors explicitly.
	UI* root = getRootUI();
	if (root == &instrumentClipView) {
		instrumentClipView.stopShortcutOverview();
	}
	else if (root == &automationView) {
		automationView.stopShortcutOverview();
	}
	row_ = 0;
	editing_ = false;
	keyboardLayoutChanged_ = false;
	focusRegained();
	return true;
}

void ScaleMenu::focusRegained() {
	indicator_leds::blinkLed(IndicatorLED::BACK);
	refresh();
}

void ScaleMenu::displayOrLanguageChanged() {
	refresh();
}

bool ScaleMenu::getGreyoutColsAndRows(uint32_t* cols, uint32_t* rows) {
	*cols = 0;
	*rows = 0;
	return true;
}

ActionResult ScaleMenu::padAction(int32_t x, int32_t y, int32_t velocity) {
	ActionResult result = getRootUI()->padAction(x, y, velocity);
	// The native gesture may open another UI. Never replace its display with this menu.
	if (getCurrentUI() == this && result != ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE) {
		display->cancelPopup();
		refresh();
	}
	return result;
}

ActionResult ScaleMenu::verticalEncoderAction(int32_t offset, bool inCardRoutine) {
	ActionResult result = getRootUI()->verticalEncoderAction(offset, inCardRoutine);
	if (getCurrentUI() == this && result != ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE) {
		display->cancelPopup();
		refresh();
	}
	return result;
}

void ScaleMenu::refresh() {
	if (display->have7SEG()) {
		const char* text = row_ == 0 ? "ROOT" : "MODE";
		if (editing_) {
			text = row_ == 0 ? rootNames[pitchClass(currentSong->key.rootNote)]
			                 : getScaleName(currentSong->getCurrentScale());
		}
		display->setScrollingText(text);
	}
	renderUIsForOled();
}

ActionResult ScaleMenu::buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) {
	using namespace deluge::hid::button;
	if (b == SELECT_ENC || b == BACK) {
		if (on) {
			if (inCardRoutine) {
				return ActionResult::REMIND_ME_OUTSIDE_CARD_ROUTINE;
			}
			if (b == BACK && !editing_) {
				close();
			}
			else {
				editing_ = b == SELECT_ENC ? !editing_ : false;
				refresh();
			}
		}
		return ActionResult::DEALT_WITH;
	}
	// Keep ordinary transport and Shift lifecycle, but do not forward sound-editing controls.
	return b == SHIFT || b == PLAY || b == RECORD ? ActionResult::NOT_DEALT_WITH : ActionResult::DEALT_WITH;
}

void ScaleMenu::changeMode(int8_t offset) {
	NoteSet present = currentSong->notesInScaleModeClips();
	present.add(0);
	std::bitset<NUM_ALL_SCALES> available;
	for (int i = 0; i < NUM_ALL_SCALES; ++i) {
		bool enabled = i == USER_SCALE ? currentSong->hasUserScale() : !currentSong->disabledPresetScales[i];
		const NoteSet& notes = i == USER_SCALE ? currentSong->getUserScaleNotes() : presetScaleNotes[i];
		available[i] = enabled && present.scaleSize() <= notes.scaleSize();
	}
	Scale candidate = stepScaleMenuMode(currentSong->getCurrentScale(), offset, available);
	if (candidate >= NUM_ALL_SCALES || !available[candidate]) {
		return;
	}
	// setScale can reject a request; preserve history until a genuine mutation succeeds.
	const NoteSet& target = candidate == USER_SCALE ? currentSong->getUserScaleNotes() : presetScaleNotes[candidate];
	if (target != currentSong->key.modeNotes && currentSong->setScale(candidate) != NO_SCALE) {
		actionLogger.deleteAllLogs();
		notifyScaleChanged();
	}
}

void ScaleMenu::notifyScaleChanged() {
	UI* root = getRootUI();
	if (root == &instrumentClipView || (root == &automationView && !automationView.onArrangerView)) {
		instrumentClipView.recalculateColours();
	}
	if (root == &keyboardScreen) {
		keyboardLayoutChanged_ |= keyboardScreen.refreshScaleMapping(true);
	}
	uiNeedsRendering(root);
	iterateAndCallSpecificDeviceHook(MIDICableUSBHosted::Hook::HOOK_ON_CHANGE_SCALE);
}

void ScaleMenu::selectEncoderAction(int8_t offset) {
	if (!offset || sdRoutineLock) {
		return;
	}
	if (editing_) {
		// Native audition releases resolve against the current key. Do not remap it during a held gesture.
		bool gestureActive = currentUIMode != UI_MODE_NONE;
		for (int32_t x = 0; x < kDisplayWidth + kSideBarWidth && !gestureActive; ++x) {
			for (int32_t y = 0; y < kDisplayHeight; ++y) {
				gestureActive |= matrixDriver.isPadPressed(x, y);
			}
		}
		if (gestureActive) {
			display->displayPopup("Release pads");
			return;
		}
	}
	if (!editing_) {
		row_ = std::clamp<int32_t>(row_ + offset, 0, 1);
	}
	else if (row_ == 0) {
		int32_t root = pitchClass(currentSong->key.rootNote + offset);
		if (root != pitchClass(currentSong->key.rootNote)) {
			actionLogger.deleteAllLogs();
			// Existing root selection preserves absolute notes and may infer a different mode.
			currentSong->setRootNote(root);
			notifyScaleChanged();
			// Root selection can change both the root and inferred mode on connected controllers.
			iterateAndCallSpecificDeviceHook(MIDICableUSBHosted::Hook::HOOK_ON_CHANGE_ROOT_NOTE);
		}
	}
	else {
		changeMode(offset);
	}
	refresh();
}

void ScaleMenu::renderOLED(deluge::hid::display::oled_canvas::Canvas& canvas) {
	canvas.clear();
	canvas.drawString(keyboardLayoutChanged_ ? "Song scale: In Key" : "Song scale", 6, 2, kTextSpacingX, kTextSpacingY);
	canvas.drawString("Root", 6, 16, kTextSpacingX, kTextSpacingY);
	canvas.drawString(rootNames[pitchClass(currentSong->key.rootNote)], 48, 16, kTextSpacingX, kTextSpacingY);
	canvas.drawString("Mode", 6, 26, kTextSpacingX, kTextSpacingY);
	canvas.drawString(getScaleName(currentSong->getCurrentScale()), 48, 26, kTextSpacingX, kTextSpacingY, 0, 127);
	int32_t y = row_ == 0 ? 16 : 26;
	canvas.invertLeftEdgeForMenuHighlighting(editing_ ? 46 : 4, editing_ ? 80 : 40, y, y + 8);
}
