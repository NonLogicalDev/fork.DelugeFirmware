#pragma once

#include "gui/ui/scale_menu_policy.h"
#include "gui/ui/ui.h"

// Song-owned Root and Mode, independent of the current Clip and SoundEditor state.
class ScaleMenu final : public UI {
public:
	ActionResult handleScaleButton(bool on, bool inCardRoutine, bool otherButtonHeld);
	bool opened() override;
	void focusRegained() override;
	void displayOrLanguageChanged() override;
	ActionResult buttonAction(deluge::hid::Button b, bool on, bool inCardRoutine) override;
	ActionResult padAction(int32_t x, int32_t y, int32_t velocity) override;
	ActionResult verticalEncoderAction(int32_t offset, bool inCardRoutine) override;
	void selectEncoderAction(int8_t offset) override;
	void renderOLED(deluge::hid::display::oled_canvas::Canvas& canvas) override;
	bool getGreyoutColsAndRows(uint32_t* cols, uint32_t* rows) override;
	void modEncoderAction(int32_t, int32_t) override {}
	void modButtonAction(uint8_t, bool) override {}
	void modEncoderButtonAction(uint8_t, bool) override {}
	UIType getUIType() override { return UIType::CONTEXT_MENU; }

private:
	void refresh();
	void notifyScaleChanged();
	bool canOpen(bool otherButtonHeld);
	void changeMode(int8_t offset);
	ScaleMenuGesture gesture_;
	bool editing_ = false;
	bool keyboardLayoutChanged_ = false;
	int32_t row_ = 0;
};

extern ScaleMenu scaleMenu;
