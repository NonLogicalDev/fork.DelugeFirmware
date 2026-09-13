#include "gui/menu_item/envelope/segment.h"
#include "gui/menu_item/envelope/time_value.h"
#include "gui/ui/sound_editor.h"
#include "hid/buttons.h"
#include "hid/display/oled.h"
#include "modulation/params/param_set.h"
#include "util/functions.h"

namespace deluge::gui::menu_item::envelope {

void Segment::readCurrentValue() {
	if (!isTime()) {
		return source::PatchedParam::readCurrentValue();
	}
	rawValue_ = soundEditor.currentParamManager->getPatchedParamSet()->getValue(getP());
	setValue(timeValueHundredths(rawValue_));
}

int32_t Segment::getFinalValue() {
	return isTime() ? rawValue_ : source::PatchedParam::getFinalValue();
}

void Segment::selectEncoderAction(int32_t offset) {
	if (!isTime()) {
		return source::PatchedParam::selectEncoderAction(offset);
	}
	if (offset == 0) {
		return;
	}
	const bool fine = Buttons::isButtonPressed(hid::button::SELECT_ENC);
	if (Buttons::isButtonPressed(hid::button::SELECT_ENC)) {
		Buttons::selectButtonPressUsedUp = true;
	}
	const int32_t next = stepTimeValue(rawValue_, offset, fine);
	if (next == rawValue_) {
		return;
	}
	rawValue_ = next;
	setValue(timeValueHundredths(rawValue_));
	// Retain the shared parameter write, Kit affect-entire, MIDI feedback and automation paths.
	Number::selectEncoderAction(offset);
}

void Segment::getNotificationValue(StringBuf& value) {
	if (!isTime()) {
		return Number::getNotificationValue(value);
	}
	value.appendFloat(getValue() / 100.f, 2, 2);
}

void Segment::drawValue() {
	if (!isTime()) {
		return source::PatchedParam::drawValue();
	}
	DEF_STACK_STRING_BUF(value, 12);
	getNotificationValue(value);
	display->setText(value, true, shouldDrawDotOnName());
}

void Segment::appendTime(StringBuf& value) {
	const uint8_t p = getP();
	// Match the unmodulated preset cable and rate conversion, not the full-width stored value.
	const int32_t combined = multiply_32x32_rshift32(rawValue_, paramRanges[p]);
	const int32_t rate = getFinalParameterValueExpWithDumbEnvelopeHack(paramNeutralValues[p], combined, p);
	if (rate <= 0) {
		value.append("infinite");
		return;
	}
	const bool attack = menu_item::PatchedParam::getP() == modulation::params::LOCAL_ENV_0_ATTACK;
	const float milliseconds = nominalTimeMilliseconds(rate, attack, kSampleRate);
	if (milliseconds < 1000.f) {
		value.appendFloat(milliseconds, milliseconds < 10.f ? 2 : 1, milliseconds < 10.f ? 2 : 1);
		value.append(" ms");
	}
	else {
		value.appendFloat(milliseconds / 1000.f, 2, 2);
		value.append(" s");
	}
}

void Segment::drawPixelsForOled() {
	if (!isTime()) {
		return source::PatchedParam::drawPixelsForOled();
	}
	DEF_STACK_STRING_BUF(value, 16);
	getNotificationValue(value);
	hid::display::OLED::main.drawStringCentred(value.c_str(), OLED_MAIN_TOPMOST_PIXEL + 15, kTextSpacingX,
	                                           kTextSpacingY);
	DEF_STACK_STRING_BUF(time, 32);
	time.append("Base ~");
	appendTime(time);
	hid::display::OLED::main.drawStringCentred(time.c_str(), OLED_MAIN_TOPMOST_PIXEL + 25, kTextSpacingX,
	                                           kTextSpacingY);
#if OLED_MAIN_VISIBLE_HEIGHT >= 42
	hid::display::OLED::main.drawStringCentred("Hold SELECT: fine", OLED_MAIN_TOPMOST_PIXEL + 35, kTextSpacingX,
	                                           kTextSpacingY);
#endif
}

void Segment::renderInHorizontalMenu(const SlotPosition& slot) {
	if (!isTime()) {
		return Number::renderInHorizontalMenu(slot);
	}
	DEF_STACK_STRING_BUF(value, 12);
	getNotificationValue(value);
	hid::display::OLED::main.drawStringCentered(value, slot.start_x, slot.start_y + kHorizontalMenuSlotYOffset,
	                                            kTextSmallSpacingX, kTextSmallSizeY, slot.width);
}

} // namespace deluge::gui::menu_item::envelope
