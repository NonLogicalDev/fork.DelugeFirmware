#include "CppUTest/TestHarness.h"
#include "gui/menu_item/envelope/time_value.h"

using namespace deluge::gui::menu_item::envelope;

TEST_GROUP(EnvelopeTime){};

TEST(EnvelopeTime, GraphPreservesLegacyRangeAndFineFractions) {
	for (int32_t whole = 0; whole <= 50; ++whole) {
		DOUBLES_EQUAL(whole, graphValue(whole * 100, 5000), 0.00001);
		DOUBLES_EQUAL(whole, graphValue(whole, 50), 0.00001);
	}
	for (int32_t fine = 0; fine <= 5000; ++fine) {
		const float value = graphValue(fine, 5000);
		CHECK(value >= 0.f && value <= 50.f);
		DOUBLES_EQUAL(fine / 100.f, value, 0.00001);
	}
	DOUBLES_EQUAL(0.01, graphValue(1, 5000), 0.00001);
}

TEST(EnvelopeTime, DisplayRangeAndMidpoint) {
	CHECK_EQUAL(0, timeValueHundredths(INT32_MIN));
	CHECK_EQUAL(2500, timeValueHundredths(0));
	CHECK_EQUAL(5000, timeValueHundredths(INT32_MAX));
}

TEST(EnvelopeTime, FineAndCoarsePreserveOffGridResidual) {
	constexpr int32_t raw = 123456789;
	CHECK_EQUAL(raw, stepTimeValue(raw, 0, false));
	CHECK_EQUAL(raw, stepTimeValue(raw, 0, true));
	CHECK_EQUAL(raw, stepTimeValue(stepTimeValue(raw, 1, false), -1, false));
	CHECK_EQUAL(raw, stepTimeValue(stepTimeValue(raw, 1, true), -1, true));
	CHECK_EQUAL(100, timeValueHundredths(stepTimeValue(raw, 1, false)) - timeValueHundredths(raw));
	CHECK_EQUAL(1, timeValueHundredths(stepTimeValue(raw, 1, true)) - timeValueHundredths(raw));
}

TEST(EnvelopeTime, SaturatesEvenForExtremeEncoderOffsets) {
	CHECK_EQUAL(INT32_MAX, stepTimeValue(0, INT32_MAX, false));
	CHECK_EQUAL(INT32_MIN, stepTimeValue(0, INT32_MIN, false));
	CHECK_EQUAL(INT32_MAX, stepTimeValue(INT32_MAX, 1, true));
	CHECK_EQUAL(INT32_MIN, stepTimeValue(INT32_MIN, -1, true));
	CHECK(stepTimeValue(INT32_MAX, -1, true) < INT32_MAX);
	CHECK(stepTimeValue(INT32_MIN, 1, true) > INT32_MIN);
}

TEST(EnvelopeTime, DisplayAndEditsAreMonotonic) {
	int32_t previous = -1;
	for (int64_t raw = INT32_MIN; raw <= INT32_MAX; raw += 858993) {
		const int32_t value = timeValueHundredths(raw);
		CHECK(value >= previous);
		CHECK(stepTimeValue(raw, 1, true) > raw);
		previous = value;
	}
}

TEST(EnvelopeTime, NominalPhaseTimeAndImmediateAttack) {
	DOUBLES_EQUAL(0.0, nominalTimeMilliseconds(245633, true, 44100), 0.0);
	CHECK(nominalTimeMilliseconds(245632, true, 44100) > 0.f);
	DOUBLES_EQUAL(46.43991, nominalTimeMilliseconds(4096, true, 44100), 0.001);
	DOUBLES_EQUAL(11888.616, nominalTimeMilliseconds(16, true, 44100), 0.01);
	DOUBLES_EQUAL(190217.87, nominalTimeMilliseconds(1, false, 44100), 0.1);
	float previous = nominalTimeMilliseconds(1, false, 44100);
	for (int32_t rate = 2; rate < 100000; rate += 103) {
		const float next = nominalTimeMilliseconds(rate, false, 44100);
		CHECK(next < previous);
		previous = next;
	}
}
