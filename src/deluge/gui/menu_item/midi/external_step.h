/*
 * Copyright © 2026 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "gui/menu_item/midi/external_step_policy.h"
#include "gui/menu_item/selection.h"
#include "gui/menu_item/submenu.h"

namespace deluge::gui::menu_item::midi::external_step {

class Clock final : public Submenu {
public:
	using Submenu::Submenu;
	bool isRelevant(ModControllableAudio* modControllable, int32_t whichThing) const override;
};

class Mode final : public Selection {
public:
	using Selection::Selection;
	void readCurrentValue() override;
	void writeCurrentValue() override;
	deluge::vector<std::string_view> getOptions(OptType optType = OptType::FULL) override;
};

class Size final : public Selection {
public:
	using Selection::Selection;
	void readCurrentValue() override;
	void writeCurrentValue() override;
	deluge::vector<std::string_view> getOptions(OptType optType = OptType::FULL) override;
	bool isRelevant(ModControllableAudio* modControllable, int32_t whichThing) const override;
};

class Input final : public MenuItem {
public:
	Input(l10n::String name, InputRole role) : MenuItem(name), role_(role) {}

	void beginSession(MenuItem* navigatedBackwardFrom = nullptr) override;
	void drawPixelsForOled() override;
	bool isRelevant(ModControllableAudio* modControllable, int32_t whichThing) const override;
	bool allowsLearnMode() override { return true; }
	bool midiLearnUsesPhysicalChannel() override { return true; }
	bool shouldBlinkLearnLed() override { return true; }
	void unlearnAction() override;
	bool learnNoteOn(MIDICable& cable, int32_t channel, int32_t noteCode) override;
	void learnProgramChange(MIDICable& cable, int32_t channel, int32_t programNumber) override;
	void learnCC(MIDICable& cable, int32_t channel, int32_t ccNumber, int32_t value) override;

private:
	void learn(MIDICable& cable, int32_t channel, deluge::midi::ExternalStepMIDIMessageType messageType,
	           int32_t number);
	void drawValue();
	void refreshAfterAssignment();

	InputRole role_;
};

class Status final : public MenuItem {
public:
	using MenuItem::MenuItem;
	void beginSession(MenuItem* navigatedBackwardFrom = nullptr) override;
	void drawPixelsForOled() override;
	bool isRelevant(ModControllableAudio* modControllable, int32_t whichThing) const override;

private:
	void drawValue();
};

} // namespace deluge::gui::menu_item::midi::external_step
