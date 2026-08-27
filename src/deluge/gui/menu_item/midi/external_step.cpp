/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "gui/menu_item/midi/external_step.h"

#include "gui/l10n/l10n.h"
#include "gui/ui/sound_editor.h"
#include "hid/display/display.h"
#include "hid/display/oled.h"
#include "io/midi/midi_device.h"
#include "model/clip/instrument_clip.h"
#include "model/model_stack.h"
#include "model/output.h"
#include "model/settings/runtime_feature_settings.h"
#include "model/song/song.h"
#include "playback/playback_handler.h"
#include "util/cfunctions.h"

namespace deluge::gui::menu_item::midi::external_step {
namespace {

[[nodiscard]] InstrumentClip* currentClip() {
	return getCurrentInstrumentClip();
}

[[nodiscard]] bool isSessionMidiOut(const InstrumentClip* clip) {
	return clip && clip->section != 255 && clip->output && clip->output->type == OutputType::MIDI_OUT;
}

[[nodiscard]] bool externalSettingsAreRelevant() {
	InstrumentClip* clip = currentClip();
	return isSessionMidiOut(clip) && clip->isExternalStepMode();
}

[[nodiscard]] deluge::midi::ExternalStepMIDIInput& inputForRole(InstrumentClip& clip, InputRole role) {
	return role == InputRole::STEP ? clip.externalStepInput : clip.externalResetInput;
}

[[nodiscard]] const deluge::midi::ExternalStepMIDIInput& otherInputForRole(const InstrumentClip& clip, InputRole role) {
	return role == InputRole::STEP ? clip.externalResetInput : clip.externalStepInput;
}

[[nodiscard]] ModelStackWithTimelineCounter* modelStackFor(InstrumentClip* clip, void* memory) {
	return setupModelStackWithTimelineCounter(memory, currentSong, clip);
}

void displayAssignmentError(l10n::String error) {
	display->displayPopup(l10n::get(error));
}

[[nodiscard]] l10n::String statusString(deluge::external_step::Status status) {
	using enum StatusLabel;
	using enum l10n::String;
	switch (statusLabel(status)) {
	case SONG:
		return STRING_FOR_SONG;
	case OFF:
		return STRING_FOR_OFF;
	case UNASSIGNED:
		return STRING_FOR_EXTERNAL_STEP_UNASSIGNED;
	case MISSING_INPUT:
		return STRING_FOR_EXTERNAL_STEP_MISSING_INPUT;
	case CONFLICT:
		return STRING_FOR_EXTERNAL_STEP_CONFLICT;
	case UNSUPPORTED:
		return STRING_FOR_EXTERNAL_STEP_UNSUPPORTED;
	case WAITING:
		return STRING_FOR_EXTERNAL_STEP_WAITING;
	case READY:
		return STRING_FOR_EXTERNAL_STEP_READY;
	}
	return STRING_FOR_EXTERNAL_STEP_UNSUPPORTED;
}

} // namespace

bool Clock::isRelevant(ModControllableAudio*, int32_t) const {
	InstrumentClip* clip = currentClip();
	return shouldShowClockMenu(isSessionMidiOut(clip),
	                           runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips),
	                           clip && clip->isExternalStepMode());
}

void Mode::readCurrentValue() {
	InstrumentClip* clip = currentClip();
	setValue(clip ? clip->externalStepClockMode : deluge::external_step::ClockMode::SONG);
}

void Mode::writeCurrentValue() {
	InstrumentClip* clip = currentClip();
	if (!isSessionMidiOut(clip)) {
		return;
	}

	auto mode = getValue<deluge::external_step::ClockMode>();
	if (mode == deluge::external_step::ClockMode::EXTERNAL_STEP
	    && !runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips)) {
		setValue(deluge::external_step::ClockMode::SONG);
		displayAssignmentError(l10n::String::STRING_FOR_OFF);
		return;
	}

	char modelStackMemory[MODEL_STACK_MAX_SIZE];
	clip->setExternalStepClockMode(mode, modelStackFor(clip, modelStackMemory));
}

deluge::vector<std::string_view> Mode::getOptions(OptType) {
	return {
	    l10n::getView(l10n::String::STRING_FOR_SONG),
	    l10n::getView(l10n::String::STRING_FOR_EXTERNAL_STEP),
	};
}

void Size::readCurrentValue() {
	InstrumentClip* clip = currentClip();
	setValue(clip ? clip->externalStepSize : deluge::external_step::StepSize::SIXTEENTH);
}

void Size::writeCurrentValue() {
	InstrumentClip* clip = currentClip();
	if (!externalSettingsAreRelevant()) {
		return;
	}

	char modelStackMemory[MODEL_STACK_MAX_SIZE];
	clip->setExternalStepSize(getValue<deluge::external_step::StepSize>(), modelStackFor(clip, modelStackMemory));
}

deluge::vector<std::string_view> Size::getOptions(OptType) {
	return {"1/4", "1/8", "1/16", "1/32"};
}

bool Size::isRelevant(ModControllableAudio*, int32_t) const {
	return externalSettingsAreRelevant();
}

void Input::beginSession(MenuItem*) {
	if (display->have7SEG()) {
		drawValue();
	}
}

void Input::drawPixelsForOled() {
	using enum deluge::midi::ExternalStepMIDIMessageType;
	deluge::hid::display::oled_canvas::Canvas& image = deluge::hid::display::OLED::main;
	InstrumentClip* clip = currentClip();
	if (!clip) {
		return;
	}

	playbackHandler.refreshExternalStepMIDIConflicts();
	const auto& input = inputForRole(*clip, role_);
	int32_t yPixel = 20;
	if (!input.isAssigned()) {
		image.drawString(l10n::get(l10n::String::STRING_FOR_EXTERNAL_STEP_UNASSIGNED), 0, yPixel, kTextSpacingX,
		                 kTextSizeYUpdated);
		return;
	}

	const char* deviceName = input.cable->getDisplayName();
	image.drawString(deviceName, 0, yPixel, kTextSpacingX, kTextSizeYUpdated);
	deluge::hid::display::OLED::setupSideScroller(0, deviceName, kTextSpacingX, OLED_MAIN_WIDTH_PIXELS, yPixel,
	                                              yPixel + 8, kTextSpacingX, kTextSpacingY, false);

	yPixel += kTextSpacingY;
	image.drawString(l10n::get(l10n::String::STRING_FOR_CHANNEL), 0, yPixel, kTextSpacingX, kTextSizeYUpdated);
	char channelBuffer[4];
	intToString(input.channel + 1, channelBuffer, 1);
	image.drawString(channelBuffer, kTextSpacingX * 8, yPixel, kTextSpacingX, kTextSizeYUpdated);

	yPixel += kTextSpacingY;
	const char* messageName = "";
	switch (input.messageType) {
	case NOTE:
		messageName = "Note";
		break;
	case CC:
		messageName = "CC";
		break;
	case PROGRAM_CHANGE:
		messageName = "PC";
		break;
	case NONE:
		break;
	}
	image.drawString(messageName, 0, yPixel, kTextSpacingX, kTextSizeYUpdated);
	char numberBuffer[4];
	intToString(input.number, numberBuffer, 1);
	image.drawString(numberBuffer, kTextSpacingX * 6, yPixel, kTextSpacingX, kTextSizeYUpdated);

	yPixel += kTextSpacingY;
	if (input.conflict) {
		image.drawString(l10n::get(l10n::String::STRING_FOR_EXTERNAL_STEP_CONFLICT), 0, yPixel, kTextSpacingX,
		                 kTextSizeYUpdated);
	}
	else if (!input.cable->connectionFlags) {
		image.drawString(l10n::get(l10n::String::STRING_FOR_EXTERNAL_STEP_MISSING_INPUT), 0, yPixel, kTextSpacingX,
		                 kTextSizeYUpdated);
	}
}

bool Input::isRelevant(ModControllableAudio*, int32_t) const {
	return externalSettingsAreRelevant();
}

void Input::unlearnAction() {
	InstrumentClip* clip = currentClip();
	if (!externalSettingsAreRelevant()) {
		return;
	}

	char modelStackMemory[MODEL_STACK_MAX_SIZE];
	ModelStackWithTimelineCounter* modelStack = modelStackFor(clip, modelStackMemory);
	clip->clearExternalStepForStop(modelStack);
	inputForRole(*clip, role_).clear();
	playbackHandler.refreshExternalStepMIDIConflicts();
	clip->validateExternalStep(modelStack,
	                           runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips));
	refreshAfterAssignment();
}

bool Input::learnNoteOn(MIDICable& cable, int32_t channel, int32_t noteCode) {
	learn(cable, channel, deluge::midi::ExternalStepMIDIMessageType::NOTE, noteCode);
	return true;
}

void Input::learnProgramChange(MIDICable& cable, int32_t channel, int32_t programNumber) {
	learn(cable, channel, deluge::midi::ExternalStepMIDIMessageType::PROGRAM_CHANGE, programNumber);
}

void Input::learnCC(MIDICable& cable, int32_t channel, int32_t ccNumber, int32_t value) {
	if (value >= 64) {
		learn(cable, channel, deluge::midi::ExternalStepMIDIMessageType::CC, ccNumber);
	}
}

void Input::learn(MIDICable& cable, int32_t channel, deluge::midi::ExternalStepMIDIMessageType messageType,
                  int32_t number) {
	InstrumentClip* clip = currentClip();
	if (!externalSettingsAreRelevant() || channel < 0 || channel >= deluge::midi::ExternalStepMIDIInput::kChannelCount
	    || number < 0 || number > UINT8_MAX
	    || !deluge::midi::ExternalStepMIDIInput::numberIsValid(messageType, static_cast<uint8_t>(number))) {
		displayAssignmentError(l10n::String::STRING_FOR_EXTERNAL_STEP_UNSUPPORTED);
		return;
	}

	deluge::midi::ExternalStepMIDIInput candidate;
	candidate.cable = &cable;
	candidate.channel = static_cast<uint8_t>(channel);
	candidate.messageType = messageType;
	candidate.number = static_cast<uint8_t>(number);
	if (assignmentsAreIdentical(candidate, otherInputForRole(*clip, role_))) {
		displayAssignmentError(l10n::String::STRING_FOR_EXTERNAL_STEP_CONFLICT);
		return;
	}

	char modelStackMemory[MODEL_STACK_MAX_SIZE];
	ModelStackWithTimelineCounter* modelStack = modelStackFor(clip, modelStackMemory);
	clip->clearExternalStepForStop(modelStack);
	auto& input = inputForRole(*clip, role_);
	input = candidate;
	if (shouldRememberHighStateAfterLearn(messageType)) {
		(void)input.updateHighState(true);
	}
	playbackHandler.refreshExternalStepMIDIConflicts();
	clip->validateExternalStep(modelStack,
	                           runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips));
	refreshAfterAssignment();
}

void Input::drawValue() {
	using enum deluge::midi::ExternalStepMIDIMessageType;
	InstrumentClip* clip = currentClip();
	if (!clip) {
		return;
	}

	const auto& input = inputForRole(*clip, role_);
	if (!input.isAssigned()) {
		display->setText(l10n::get(l10n::String::STRING_FOR_EXTERNAL_STEP_UNASSIGNED));
		return;
	}

	char text[5];
	switch (input.messageType) {
	case NOTE:
		text[0] = 'N';
		break;
	case CC:
		text[0] = 'C';
		break;
	case PROGRAM_CHANGE:
		text[0] = 'P';
		break;
	case NONE:
		text[0] = '-';
		break;
	}
	intToString(input.number, &text[1], 3);
	display->setText(text);
}

void Input::refreshAfterAssignment() {
	if (soundEditor.getCurrentMenuItem() == this) {
		if (display->haveOLED()) {
			renderUIsForOled();
		}
		else {
			drawValue();
		}
	}
	else {
		display->displayPopup(l10n::get(l10n::String::STRING_FOR_LEARNED));
	}
}

void Status::beginSession(MenuItem*) {
	if (display->have7SEG()) {
		drawValue();
	}
}

void Status::drawPixelsForOled() {
	InstrumentClip* clip = currentClip();
	if (!clip) {
		return;
	}

	playbackHandler.refreshExternalStepMIDIConflicts();
	auto status = clip->getExternalStepStatus(
	    runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips), currentSong);
	deluge::hid::display::OLED::main.drawString(l10n::get(statusString(status)), 0, 20, kTextSpacingX,
	                                            kTextSizeYUpdated);
}

bool Status::isRelevant(ModControllableAudio*, int32_t) const {
	return externalSettingsAreRelevant();
}

void Status::drawValue() {
	InstrumentClip* clip = currentClip();
	if (!clip) {
		return;
	}

	playbackHandler.refreshExternalStepMIDIConflicts();
	auto status = clip->getExternalStepStatus(
	    runtimeFeatureSettings.isOn(RuntimeFeatureSettingType::ExternalStepMidiClips), currentSong);
	display->setText(l10n::get(statusString(status)));
}

} // namespace deluge::gui::menu_item::midi::external_step
