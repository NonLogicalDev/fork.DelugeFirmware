#ifdef DELUGE_OLED_WAVEFORM_BUCKET_COUNT
#define DELUGE_OLED_WAVEFORM_TESTS_EXPLICIT_BUCKET_COUNT 1
#else
#define DELUGE_OLED_WAVEFORM_TESTS_EXPLICIT_BUCKET_COUNT 0
#endif

#include "gui/waveform/oled_waveform_renderer.h"

#include "cppspec.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace {

class RecordingCanvas {
public:
	void drawHorizontalLine(int32_t y, int32_t startX, int32_t endX) {
		lineCalls++;
		if (y < 0 || y >= OLED_MAIN_HEIGHT_PIXELS || startX < 0 || endX >= OLED_MAIN_WIDTH_PIXELS || endX < startX) {
			outOfBounds = true;
			return;
		}
		for (int32_t x = startX; x <= endX; x++) {
			pixels[y][x] = true;
			pixelWrites++;
		}
	}

	[[nodiscard]] bool lit(int32_t x, int32_t y) const { return pixels[y][x]; }

	[[nodiscard]] int32_t litPixels() const {
		int32_t total = 0;
		for (auto const& row : pixels) {
			for (bool pixel : row) {
				total += pixel;
			}
		}
		return total;
	}

	std::array<std::array<bool, OLED_MAIN_WIDTH_PIXELS>, OLED_MAIN_HEIGHT_PIXELS> pixels{};
	int32_t lineCalls = 0;
	int32_t pixelWrites = 0;
	bool outOfBounds = false;
};

deluge::gui::waveform::OledWaveformRenderData peaksWithAllBuckets(int32_t minValue, int32_t maxValue) {
	deluge::gui::waveform::OledWaveformRenderData data{};
	for (size_t bucket = 0; bucket < deluge::gui::waveform::kOledWaveformBucketCount; bucket++) {
		data.minPerCol[bucket] = minValue;
		data.maxPerCol[bucket] = maxValue;
		data.colStatus[bucket] = COL_STATUS_INVESTIGATED;
	}
	return data;
}

} // namespace

using deluge::gui::waveform::aggregateOledWaveformToPadColumns;
using deluge::gui::waveform::kOledWaveformBucketCount;
using deluge::gui::waveform::oledWaveformBucketCountIsValid;
using deluge::gui::waveform::oledWaveformBucketSampleRange;
using deluge::gui::waveform::oledWaveformCacheMatches;
using deluge::gui::waveform::oledWaveformInvestigatedBucketCount;
using deluge::gui::waveform::oledWaveformPendingBucketCount;
using deluge::gui::waveform::oledWaveformPrepareResult;
using deluge::gui::waveform::OledWaveformRenderData;
using deluge::gui::waveform::OledWaveformViewport;
using deluge::gui::waveform::renderOledWaveformContour;
using deluge::gui::waveform::updateOledWaveformCacheViewport;

#if !DELUGE_OLED_WAVEFORM_TESTS_EXPLICIT_BUCKET_COUNT
static_assert(kOledWaveformBucketCount == 128);
static_assert(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount == 1);
static_assert(sizeof(OledWaveformRenderData) == 1168);
#endif
static_assert(oledWaveformBucketCountIsValid(16));
static_assert(oledWaveformBucketCountIsValid(32));
static_assert(oledWaveformBucketCountIsValid(64));
static_assert(oledWaveformBucketCountIsValid(128));
static_assert(OLED_MAIN_WIDTH_PIXELS / 16 == 8);
static_assert(OLED_MAIN_WIDTH_PIXELS / 32 == 4);
static_assert(OLED_MAIN_WIDTH_PIXELS / 64 == 2);
static_assert(OLED_MAIN_WIDTH_PIXELS / 128 == 1);
static_assert(!oledWaveformBucketCountIsValid(0));
static_assert(!oledWaveformBucketCountIsValid(8));
static_assert(!oledWaveformBucketCountIsValid(48));
static_assert(!oledWaveformBucketCountIsValid(129));
static_assert(std::is_standard_layout_v<OledWaveformRenderData>);
static_assert(std::is_trivially_copyable_v<OledWaveformRenderData>);
static_assert(sizeof(OledWaveformRenderData) == 16 + 9 * kOledWaveformBucketCount);

// clang-format off
describe oled_waveform_renderer("OLED waveform renderer", $ {
	it("partitions a non-divisible viewport without gaps or overlaps", _ {
		constexpr int64_t scroll = 1234;
		constexpr uint64_t zoom = 63;
		constexpr uint64_t span = zoom * kDisplayWidth;
		constexpr uint64_t narrowRange = span / kOledWaveformBucketCount;
		constexpr uint64_t wideRange = narrowRange + 1;
		constexpr int32_t expectedWiderRanges = span % kOledWaveformBucketCount;
		int64_t expectedStart = scroll;
		int32_t widerRanges = 0;

		for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
			const auto range = oledWaveformBucketSampleRange(scroll, zoom, bucket, kOledWaveformBucketCount);
			expect(range.valid).to_be_true();
			expect(range.start).to_equal(expectedStart);
			expect(range.end == range.start + static_cast<int64_t>(wideRange)
			       || range.end == range.start + static_cast<int64_t>(narrowRange))
			    .to_be_true();
			widerRanges += range.end - range.start == static_cast<int64_t>(wideRange);
			expectedStart = range.end;
		}

		expect(widerRanges).to_equal(expectedWiderRanges);
		expect(expectedStart).to_equal(scroll + static_cast<int64_t>(span));
	});

	it("keeps every OLED bucket group aligned with its pad column", _ {
		constexpr int64_t scroll = 321;
		constexpr std::array<size_t, 4> bucketCounts{16, 32, 64, 128};
		constexpr std::array<uint64_t, 7> zooms{1, 3, 5, 9, 10, 17, 63};

		for (size_t bucketCount : bucketCounts) {
			const size_t bucketsPerPadColumn = bucketCount / kDisplayWidth;
			for (uint64_t zoom : zooms) {
				const uint64_t viewportSpan = zoom * kDisplayWidth;
				for (size_t padColumn = 0; padColumn < kDisplayWidth; padColumn++) {
					const size_t firstBucket = padColumn * bucketsPerPadColumn;
					const size_t lastBucket = (padColumn + 1) * bucketsPerPadColumn - 1;
					const auto first = oledWaveformBucketSampleRange(scroll, zoom, firstBucket, bucketCount);
					const auto last = oledWaveformBucketSampleRange(scroll, zoom, lastBucket, bucketCount);
					expect(first.valid).to_be_true();
					expect(last.valid).to_be_true();
					expect(first.start).to_equal(scroll + static_cast<int64_t>(padColumn * zoom));
					expect(last.end).to_equal(scroll + static_cast<int64_t>((padColumn + 1) * zoom));
				}

				if (viewportSpan >= bucketCount) {
					int64_t expectedStart = scroll;
					for (size_t bucket = 0; bucket < bucketCount; bucket++) {
						const auto range = oledWaveformBucketSampleRange(scroll, zoom, bucket, bucketCount);
						expect(range.valid).to_be_true();
						expect(range.start).to_equal(expectedStart);
						expectedStart = range.end;
					}
					expect(expectedStart).to_equal(scroll + static_cast<int64_t>(viewportSpan));
				}
			}
		}
	});

	it("measures every non-tight sample in the bucket that displays it", _ {
		constexpr int64_t scroll = 321;
		constexpr std::array<size_t, 4> bucketCounts{16, 32, 64, 128};
		constexpr std::array<uint64_t, 4> zooms{9, 10, 17, 63};

		for (size_t bucketCount : bucketCounts) {
			const size_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / bucketCount;
			for (uint64_t zoom : zooms) {
				const OledWaveformViewport viewport{scroll, static_cast<int64_t>(zoom)};
				const uint64_t viewportSpan = zoom * kDisplayWidth;
				for (uint64_t offset = 0; offset < viewportSpan; offset++) {
					const int32_t x = viewport.samplePositionToX(scroll + static_cast<int64_t>(offset));
					expect(x >= 0).to_be_true();
					const size_t bucket = static_cast<size_t>(x) / pixelsPerBucket;
					const auto range = oledWaveformBucketSampleRange(scroll, zoom, bucket, bucketCount);
					expect(range.valid).to_be_true();
					expect(range.start <= scroll + static_cast<int64_t>(offset)).to_be_true();
					expect(range.end > scroll + static_cast<int64_t>(offset)).to_be_true();
				}
			}
		}
	});

	it("explicitly repeats sample ownership in a viewport tighter than the cache", _ {
		constexpr int64_t scroll = 900;
		constexpr uint64_t zoom = 1;
		std::array<int32_t, kDisplayWidth> owners{};

		for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
			const auto range = oledWaveformBucketSampleRange(scroll, zoom, bucket, kOledWaveformBucketCount);
			expect(range.valid).to_be_true();
			expect(range.end).to_equal(range.start + 1);
			expect(range.start >= scroll && range.start < scroll + kDisplayWidth).to_be_true();
			owners[range.start - scroll]++;
		}

		for (int32_t ownerCount : owners) {
			expect(ownerCount).to_equal(static_cast<int32_t>(kOledWaveformBucketCount / kDisplayWidth));
		}
	});

	it("rejects invalid bucket partitions and signed-position overflow", _ {
		expect(oledWaveformBucketSampleRange(0, 0, 0, kOledWaveformBucketCount).valid).to_be_false();
		expect(oledWaveformBucketSampleRange(0, 1, 0, 48).valid).to_be_false();
		expect(oledWaveformBucketSampleRange(0, 1, kOledWaveformBucketCount, kOledWaveformBucketCount).valid)
		    .to_be_false();
		expect(oledWaveformBucketSampleRange(std::numeric_limits<int64_t>::max(), 1,
		                                         kOledWaveformBucketCount - 1, kOledWaveformBucketCount)
		           .valid)
		    .to_be_false();
	});

	it("supports the largest editor viewport and positions beyond signed 32-bit", _ {
		const auto atInt32Zoom =
		    oledWaveformBucketSampleRange(0, std::numeric_limits<int32_t>::max(), 0, kOledWaveformBucketCount);
		const auto beyondInt32Zoom = oledWaveformBucketSampleRange(
		    0, uint64_t{std::numeric_limits<int32_t>::max()} + 1, 0, kOledWaveformBucketCount);
		constexpr uint64_t int32ZoomSpan = uint64_t{std::numeric_limits<int32_t>::max()} * kDisplayWidth;
		constexpr uint64_t beyondInt32ZoomSpan =
		    (uint64_t{std::numeric_limits<int32_t>::max()} + 1) * kDisplayWidth;
		expect(atInt32Zoom.valid).to_be_true();
		expect(atInt32Zoom.start).to_equal(int64_t{0});
		expect(atInt32Zoom.end)
		    .to_equal(static_cast<int64_t>((int32ZoomSpan + kOledWaveformBucketCount - 1)
		                                   / kOledWaveformBucketCount));
		expect(beyondInt32Zoom.valid).to_be_true();
		expect(beyondInt32Zoom.start).to_equal(int64_t{0});
		expect(beyondInt32Zoom.end)
		    .to_equal(static_cast<int64_t>((beyondInt32ZoomSpan + kOledWaveformBucketCount - 1)
		                                   / kOledWaveformBucketCount));

		constexpr uint64_t largestZoom = (uint64_t{std::numeric_limits<uint32_t>::max()} + 1) / kDisplayWidth;
		constexpr uint64_t largestSpan = largestZoom * kDisplayWidth;
		const auto first = oledWaveformBucketSampleRange(0, largestZoom, 0, kOledWaveformBucketCount);
		const auto last = oledWaveformBucketSampleRange(0, largestZoom, kOledWaveformBucketCount - 1,
		                                                kOledWaveformBucketCount);
		expect(first.valid).to_be_true();
		expect(first.start).to_equal(int64_t{0});
		expect(first.end)
		    .to_equal(static_cast<int64_t>((largestSpan + kOledWaveformBucketCount - 1)
		                                   / kOledWaveformBucketCount));
		expect(last.valid).to_be_true();
		expect(last.start)
		    .to_equal(static_cast<int64_t>(largestSpan - largestSpan / kOledWaveformBucketCount));
		expect(last.end).to_equal(static_cast<int64_t>(largestSpan));

		constexpr int64_t largeScroll = int64_t{std::numeric_limits<int32_t>::max()} + 1;
		const auto beyondInt32 = oledWaveformBucketSampleRange(largeScroll, 8, 0, kOledWaveformBucketCount);
		expect(beyondInt32.valid).to_be_true();
		expect(beyondInt32.start).to_equal(largeScroll);
		expect(beyondInt32.end)
		    .to_equal(largeScroll + static_cast<int64_t>((8 * kDisplayWidth + kOledWaveformBucketCount - 1)
		                                               / kOledWaveformBucketCount));
	});

	it("folds OLED extrema and statuses into the existing pad cache", _ {
		constexpr size_t bucketsPerPadColumn = kOledWaveformBucketCount / kDisplayWidth;
		constexpr size_t secondPadStart = bucketsPerPadColumn;
		OledWaveformRenderData source{};
		source.xScroll = 4321;
		source.xZoom = 77;
		for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
			source.colStatus[bucket] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
		}

		source.colStatus[0] = COL_STATUS_INVESTIGATED;
		source.minPerCol[0] = -30;
		source.maxPerCol[0] = 40;
		if constexpr (bucketsPerPadColumn > 1) {
			source.minPerCol[0] = -10;
			source.maxPerCol[0] = 20;
			source.colStatus[1] = COL_STATUS_INVESTIGATED;
			source.minPerCol[1] = -30;
			source.maxPerCol[1] = 40;

			source.colStatus[secondPadStart] = COL_STATUS_INVESTIGATED;
			source.minPerCol[secondPadStart] = -100;
			source.maxPerCol[secondPadStart] = 100;
			source.colStatus[secondPadStart + 1] = 0;
		}
		else {
			source.colStatus[secondPadStart] = 0;
		}

		WaveformRenderData destination{};
		aggregateOledWaveformToPadColumns(source, destination);

		expect(destination.xScroll).to_equal(source.xScroll);
		expect(destination.xZoom).to_equal(source.xZoom);
		expect(destination.colStatus[0]).to_equal(COL_STATUS_INVESTIGATED);
		expect(destination.minPerCol[0]).to_equal(-30);
		expect(destination.maxPerCol[0]).to_equal(40);
		expect(destination.colStatus[1]).to_equal(uint8_t{0});
		expect(destination.colStatus[2]).to_equal(COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM);
	});

	it("keeps a matching cache key and retries only unresolved buckets", _ {
		OledWaveformRenderData data{};
		data.xScroll = 100;
		data.xZoom = 20;
		for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
			data.colStatus[bucket] = COL_STATUS_INVESTIGATED;
		}
		data.colStatus[5] = 0;
		data.colStatus[12] = 0;
		data.colStatus[kOledWaveformBucketCount - 1] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;

		expect(oledWaveformCacheMatches(data, 100, 20)).to_be_true();
		expect(updateOledWaveformCacheViewport(data, 100, 20)).to_be_false();
		expect(oledWaveformPendingBucketCount(data)).to_equal(size_t{2});
		expect(oledWaveformInvestigatedBucketCount(data)).to_equal(kOledWaveformBucketCount - 3);
		expect(data.colStatus[0]).to_equal(COL_STATUS_INVESTIGATED);
		expect(data.colStatus[kOledWaveformBucketCount - 1])
		    .to_equal(COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM);

		data.colStatus[5] = COL_STATUS_INVESTIGATED;
		expect(updateOledWaveformCacheViewport(data, 100, 20)).to_be_false();
		expect(oledWaveformPendingBucketCount(data)).to_equal(size_t{1});
		expect(data.colStatus[5]).to_equal(COL_STATUS_INVESTIGATED);
	});

	it("invalidates all buckets once on a viewport cache miss", _ {
		OledWaveformRenderData data = peaksWithAllBuckets(-100, 100);
		data.xScroll = 100;
		data.xZoom = 20;

		expect(updateOledWaveformCacheViewport(data, 101, 20)).to_be_true();
		expect(oledWaveformCacheMatches(data, 101, 20)).to_be_true();
		expect(oledWaveformPendingBucketCount(data)).to_equal(kOledWaveformBucketCount);
		expect(kOledWaveformBucketCount <= OLED_MAIN_WIDTH_PIXELS).to_be_true();

		expect(updateOledWaveformCacheViewport(data, 101, 20)).to_be_false();
		expect(oledWaveformPendingBucketCount(data)).to_equal(kOledWaveformBucketCount);
	});

	it("reports cache progress without treating unchanged retries as OLED changes", _ {
		const auto newViewport = oledWaveformPrepareResult(false, 0, 0, false);
		expect(newViewport.complete).to_be_false();
		expect(newViewport.cacheChanged).to_be_true();

		const auto unchangedFailure =
		    oledWaveformPrepareResult(true, kOledWaveformBucketCount, kOledWaveformBucketCount, false);
		expect(unchangedFailure.complete).to_be_false();
		expect(unchangedFailure.cacheChanged).to_be_false();

		const auto beyondOnlyProgress = oledWaveformPrepareResult(true, 10, 10, false);
		expect(beyondOnlyProgress.complete).to_be_false();
		expect(beyondOnlyProgress.cacheChanged).to_be_false();

		const auto partialProgress = oledWaveformPrepareResult(true, 10, 11, false);
		expect(partialProgress.complete).to_be_false();
		expect(partialProgress.cacheChanged).to_be_true();

		const auto completed = oledWaveformPrepareResult(true, kOledWaveformBucketCount - 1,
		                                                     kOledWaveformBucketCount, true);
		expect(completed.complete).to_be_true();
		expect(completed.cacheChanged).to_be_true();

		const auto completeCacheHit = oledWaveformPrepareResult(true, 0, 0, true);
		expect(completeCacheHit.complete).to_be_true();
		expect(completeCacheHit.cacheChanged).to_be_false();
	});

	it("draws at most two contour lines per measured bucket", _ {
		RecordingCanvas canvas;
		const OledWaveformRenderData data = peaksWithAllBuckets(-100, 100);

		renderOledWaveformContour(canvas, data, -100, 100, 5, 47);

		expect(canvas.lineCalls).to_equal(static_cast<int32_t>(kOledWaveformBucketCount * 2));
		expect(canvas.pixelWrites).to_equal(256);
		expect(canvas.litPixels()).to_equal(256);
		expect(canvas.outOfBounds).to_be_false();
		for (int32_t x = 0; x < OLED_MAIN_WIDTH_PIXELS; x++) {
			expect(canvas.lit(x, 5)).to_be_true();
			expect(canvas.lit(x, 47)).to_be_true();
			expect(canvas.lit(x, 26)).to_be_false();
		}
	});

	it("renders an isolated measured bucket without interpolation", _ {
		constexpr int32_t bucket = kOledWaveformBucketCount / 4;
		constexpr int32_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount;
		constexpr int32_t startX = bucket * pixelsPerBucket;
		constexpr int32_t endX = startX + pixelsPerBucket - 1;
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[bucket] = -100;
		data.maxPerCol[bucket] = 100;
		data.colStatus[bucket] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, -100, 100, 5, 47);

		expect(canvas.lineCalls).to_equal(2);
		expect(canvas.pixelWrites).to_equal(pixelsPerBucket * 2);
		expect(canvas.lit(startX - 1, 5)).to_be_false();
		expect(canvas.lit(startX, 5)).to_be_true();
		expect(canvas.lit(endX, 47)).to_be_true();
		expect(canvas.lit(endX + 1, 47)).to_be_false();
	});

	it("maps values deterministically and leaves incomplete bins blank", _ {
		constexpr int32_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount;
		constexpr int32_t startX = 2 * pixelsPerBucket;
		constexpr int32_t endX = startX + pixelsPerBucket;
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[2] = -50;
		data.maxPerCol[2] = 50;
		data.colStatus[2] = COL_STATUS_INVESTIGATED;
		data.colStatus[3] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;

		renderOledWaveformContour(canvas, data, -100, 100, 10, 30);

		expect(canvas.lineCalls).to_equal(2);
		expect(canvas.pixelWrites).to_equal(pixelsPerBucket * 2);
		expect(canvas.litPixels()).to_equal(pixelsPerBucket * 2);
		for (int32_t x = startX; x < endX; x++) {
			expect(canvas.lit(x, 15)).to_be_true();
			expect(canvas.lit(x, 25)).to_be_true();
		}
		expect(canvas.lit(endX, 15)).to_be_false();
		expect(canvas.lit(4 * pixelsPerBucket, 25)).to_be_false();
	});

	it("reverses bins without mutating the cache", _ {
		constexpr int32_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount;
		constexpr int32_t reversedStartX = OLED_MAIN_WIDTH_PIXELS - pixelsPerBucket;
		OledWaveformRenderData data{};
		data.minPerCol[0] = -100;
		data.maxPerCol[0] = 100;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;

		RecordingCanvas forwards;
		renderOledWaveformContour(forwards, data, -100, 100, 5, 47);
		expect(forwards.lit(0, 5)).to_be_true();
		expect(forwards.lit(pixelsPerBucket - 1, 47)).to_be_true();
		expect(forwards.lit(pixelsPerBucket, 5)).to_be_false();

		RecordingCanvas reversed;
		renderOledWaveformContour(reversed, data, -100, 100, 5, 47, true);
		expect(reversed.lit(reversedStartX - 1, 5)).to_be_false();
		expect(reversed.lit(reversedStartX, 5)).to_be_true();
		expect(reversed.lit(127, 47)).to_be_true();
		expect(data.colStatus[0]).to_equal(COL_STATUS_INVESTIGATED);
	});

	it("clips the requested plot to the visible panel", _ {
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = -100;
		data.maxPerCol[0] = 100;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, -100, 100, std::numeric_limits<int32_t>::min(),
		                          std::numeric_limits<int32_t>::max());

		expect(canvas.outOfBounds).to_be_false();
		expect(canvas.lit(0, OLED_MAIN_TOPMOST_PIXEL)).to_be_true();
		expect(canvas.lit(0, OLED_MAIN_HEIGHT_PIXELS - 1)).to_be_true();
	});

	it("does no work for an empty plot or inverted extrema", _ {
		const OledWaveformRenderData data = peaksWithAllBuckets(-100, 100);

		RecordingCanvas emptyPlot;
		renderOledWaveformContour(emptyPlot, data, -100, 100, 40, 20);
		expect(emptyPlot.lineCalls).to_equal(0);

		RecordingCanvas invertedExtrema;
		renderOledWaveformContour(invertedExtrema, data, 10, -10, 5, 47);
		expect(invertedExtrema.lineCalls).to_equal(0);
	});

	it("does not write a contour twice when both peaks quantize to one row", _ {
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = -1;
		data.maxPerCol[0] = 1;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, -100, 100, 20, 20);

		expect(canvas.lineCalls).to_equal(1);
		expect(canvas.pixelWrites).to_equal(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount);
		expect(canvas.litPixels()).to_equal(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount);
	});

	it("maps exact interior ratios without reciprocal rounding error", _ {
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = 3;
		data.maxPerCol[0] = 3;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, 0, 6, 10, 12);

		expect(canvas.lineCalls).to_equal(1);
		expect(canvas.lit(0, 11)).to_be_true();
		expect(canvas.lit(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount, 11)).to_be_false();
	});

	it("maps the full signed sample range without overflow", _ {
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = 0;
		data.maxPerCol[0] = 0;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, std::numeric_limits<int32_t>::min(),
		                          std::numeric_limits<int32_t>::max(), 5, 47);

		expect(canvas.lineCalls).to_equal(1);
		expect(canvas.lit(0, 26)).to_be_true();
		expect(canvas.lit(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount, 26)).to_be_false();
		expect(canvas.outOfBounds).to_be_false();
	});

	it("renders investigated silence as a centered flat contour", _ {
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = 0;
		data.maxPerCol[0] = 0;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;
		data.colStatus[1] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;

		renderOledWaveformContour(canvas, data, 0, 0, 5, 47);

		expect(canvas.lineCalls).to_equal(1);
		expect(canvas.pixelWrites).to_equal(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount);
		expect(canvas.lit(0, 26)).to_be_true();
		expect(canvas.lit(OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount, 26)).to_be_false();
	});

	it("maps asymmetric extrema and peaks exactly", _ {
		constexpr int32_t pixelsPerBucket = OLED_MAIN_WIDTH_PIXELS / kOledWaveformBucketCount;
		RecordingCanvas canvas;
		OledWaveformRenderData data{};
		data.minPerCol[0] = -100;
		data.maxPerCol[0] = 100;
		data.minPerCol[1] = 0;
		data.maxPerCol[1] = 300;
		data.colStatus[0] = COL_STATUS_INVESTIGATED;
		data.colStatus[1] = COL_STATUS_INVESTIGATED;

		renderOledWaveformContour(canvas, data, -100, 300, 5, 45);

		expect(canvas.lit(0, 45)).to_be_true();
		expect(canvas.lit(pixelsPerBucket - 1, 25)).to_be_true();
		expect(canvas.lit(pixelsPerBucket, 35)).to_be_true();
		expect(canvas.lit(2 * pixelsPerBucket - 1, 5)).to_be_true();
		expect(canvas.lineCalls).to_equal(4);
		expect(canvas.pixelWrites).to_equal(pixelsPerBucket * 4);
	});

	it("maps viewport starts, ends, and offscreen positions with exclusive end semantics", _ {
		OledWaveformRenderData data{};
		data.xScroll = int64_t{std::numeric_limits<int32_t>::max()} + 100;
		data.xZoom = 10;
		const OledWaveformViewport viewport{data};

		expect(viewport.span()).to_equal(uint64_t{160});
		expect(viewport.samplePositionToX(data.xScroll)).to_equal(0);
		expect(viewport.samplePositionToX(data.xScroll + 159)).to_equal(127);
		expect(viewport.samplePositionToX(data.xScroll - 1)).to_equal(-1);
		expect(viewport.samplePositionToX(data.xScroll + 160)).to_equal(-1);
		expect(viewport.samplePositionToX(data.xScroll, true)).to_equal(-1);
		expect(viewport.samplePositionToX(data.xScroll + 1, true)).to_equal(0);
		expect(viewport.samplePositionToX(data.xScroll + 160, true)).to_equal(127);
		expect(viewport.samplePositionToX(data.xScroll + 161, true)).to_equal(-1);
	});

	it("rejects invalid or unsupported viewport spans", _ {
		OledWaveformRenderData zeroZoom{};
		zeroZoom.xZoom = 0;
		const OledWaveformViewport empty{zeroZoom};
		expect(empty.span()).to_equal(uint64_t{0});
		expect(empty.samplePositionToX(0)).to_equal(-1);

		OledWaveformRenderData negativeZoom{};
		negativeZoom.xZoom = -1;
		const OledWaveformViewport negative{negativeZoom};
		expect(negative.span()).to_equal(uint64_t{0});

		OledWaveformRenderData hugeZoom{};
		hugeZoom.xZoom = std::numeric_limits<int64_t>::max();
		const OledWaveformViewport unsupported{hugeZoom};
		expect(unsupported.span()).to_equal(uint64_t{0});
		expect(unsupported.samplePositionToX(std::numeric_limits<int64_t>::max())).to_equal(-1);
	});

	it("supports the navigator's largest possible viewport", _ {
		OledWaveformRenderData data{};
		data.xZoom = (uint64_t{std::numeric_limits<uint32_t>::max()} + 1) / kDisplayWidth;
		const OledWaveformViewport viewport{data};

		expect(viewport.span()).to_equal(uint64_t{std::numeric_limits<uint32_t>::max()} + 1);
		expect(viewport.samplePositionToX(std::numeric_limits<uint32_t>::max())).to_equal(127);
		expect(viewport.samplePositionToX(uint64_t{std::numeric_limits<uint32_t>::max()} + 1)).to_equal(-1);
	});
});

CPPSPEC_SPEC(oled_waveform_renderer)
