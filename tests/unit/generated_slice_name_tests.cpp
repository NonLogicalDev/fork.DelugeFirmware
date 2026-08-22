#include "CppUTest/TestHarness.h"

#include "model/drum/generated_slice_name.h"

TEST_GROUP(GeneratedSliceNameTests){};

TEST(GeneratedSliceNameTests, identifiesSeriesIndicesCaseInsensitively) {
	LONGS_EQUAL(0, deluge::generated_slice_name::getSeriesIndex("A-01"));
	LONGS_EQUAL(1, deluge::generated_slice_name::getSeriesIndex("b-12"));
	LONGS_EQUAL(25, deluge::generated_slice_name::getSeriesIndex("Z-64"));
	LONGS_EQUAL(26, deluge::generated_slice_name::getSeriesIndex("AA-01"));
}

TEST(GeneratedSliceNameTests, rejectsNamesOutsideTheGeneratedFormat) {
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("1"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("A-1"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("A-00"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("A-001"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("A-257"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("A-01 take"));
	LONGS_EQUAL(-1, deluge::generated_slice_name::getSeriesIndex("Kick"));
}

TEST(GeneratedSliceNameTests, formatsZeroPaddedPartsAndRollsPastZ) {
	STRCMP_EQUAL("A-01", deluge::generated_slice_name::makeName(0, 1).c_str());
	STRCMP_EQUAL("B-12", deluge::generated_slice_name::makeName(1, 12).c_str());
	STRCMP_EQUAL("AA-256", deluge::generated_slice_name::makeName(26, 256).c_str());
}
