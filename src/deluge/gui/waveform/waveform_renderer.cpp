/*
 * Copyright © 2018-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/waveform/waveform_renderer.h"
#include "definitions_cxx.hpp"
#include "gui/colour/colour.h"
#include "gui/waveform/oled_waveform_render_data.h"
#include "gui/waveform/waveform_render_data.h"
#include "io/debug/log.h"
#include "model/sample/sample.h"
#include "model/sample/sample_recorder.h"
#include "model/voice/voice_sample.h"
#include "processing/engines/audio_engine.h"
#include "scheduler_api.h"
#include "storage/audio/audio_file_manager.h"
#include "storage/cluster/cluster.h"
#include "storage/multi_range/multisample_range.h"
#include <optional>
#include <string.h>

extern "C" {
extern uint8_t currentlyAccessingCard;
}

WaveformRenderer waveformRenderer{};

WaveformRenderer::WaveformRenderer() {
}

#define SAMPLES_TO_READ_PER_COL_MAGNITUDE 9
static_assert(Cluster::kSizeFAT16Max <= std::numeric_limits<int32_t>::max());

// Returns false if had trouble loading some (will often not be all) Clusters, e.g. cos we're in the card routine
bool WaveformRenderer::renderFullScreen(Sample* sample, int64_t xScroll, uint64_t xZoom,
                                        RGB thisImage[][kDisplayWidth + kSideBarWidth], WaveformRenderData* data,
                                        SampleRecorder* recorder, std::optional<RGB> rgb, bool reversed, int32_t xEnd) {

	bool completeSuccess = findPeaksPerCol(sample, xScroll, xZoom, data, recorder);
	if (!completeSuccess) {
		return false;
	}
	renderFullScreenFromData(sample, thisImage, data, rgb, reversed, xEnd);
	return true;
}

void WaveformRenderer::renderFullScreenFromData(Sample* sample, RGB thisImage[][kDisplayWidth + kSideBarWidth],
                                                WaveformRenderData* data, std::optional<RGB> rgb, bool reversed,
                                                int32_t xEnd) {

	// Clear display
	for (int32_t y = 0; y < kDisplayHeight; y++) {
		memset(thisImage[y], 0, kDisplayWidth * 3);
	}

	for (int32_t xDisplay = 0; xDisplay < xEnd; xDisplay++) {
		renderOneCol(sample, xDisplay, thisImage, data, reversed, rgb);
	}
}

// Returns false if had trouble loading some (will often not be all) Clusters
bool WaveformRenderer::renderAsSingleRow(Sample* sample, int64_t xScroll, uint64_t xZoom, RGB* thisImage,
                                         WaveformRenderData* data, SampleRecorder* recorder, RGB rgb, bool reversed,
                                         int32_t xStart, int32_t xEnd) {

	int32_t xStartSource = xStart;
	int32_t xEndSource = xEnd;
	if (reversed) {
		const auto sourceRange = deluge::gui::waveform::reverseWaveformColumnRange(xStart, xEnd, kDisplayWidth);
		xStartSource = sourceRange.start;
		xEndSource = sourceRange.end;
	}

	bool completeSuccess = findPeaksPerCol(sample, xScroll, xZoom, data, recorder, xStartSource, xEndSource);
	if (!completeSuccess) {
		return false;
	}

	int32_t maxPeakFromZero = sample->getMaxPeakFromZero();

	for (int32_t xDisplayOutput = xStart; xDisplayOutput < xEnd; xDisplayOutput++) {

		int32_t xDisplaySource = xDisplayOutput;
		if (reversed) {
			xDisplaySource = kDisplayWidth - 1 - xDisplaySource;
		}

		// If no data here (e.g. if Sample not recorded this far yet...)
		if (data->colStatus[xDisplaySource] != COL_STATUS_INVESTIGATED) {
			thisImage[xDisplayOutput] = deluge::gui::colours::black;
			continue;
		}

		int32_t colourValue = getColBrightnessForSingleRow(xDisplaySource, maxPeakFromZero, data);
		colourValue = (colourValue * colourValue); // >> 8;
		// if (colourValue > 255) colourValue = 255; // May sometimes go juuust over

		thisImage[xDisplayOutput] = rgb.transform([colourValue](auto channel) {
			int32_t valueHere = (colourValue * channel) >> 16;
			// Limit the heck out of the bit depth, to avoid problem with PIC firmware where too many different colour
			// shades cause big problems. The 6 is quite arbitrary, but I think it looks good
			return std::clamp<int32_t>((valueHere + 6) & ~15, 0, RGB::channel_max);
		});
	}

	return true;
}

// Value out of 255
int32_t WaveformRenderer::getColBrightnessForSingleRow(int32_t xDisplay, int32_t maxPeakFromZero,
                                                       WaveformRenderData* data) {

	int32_t peak1 = std::abs(data->minPerCol[xDisplay]);
	int32_t peak2 = std::abs(data->maxPerCol[xDisplay]);

	int32_t peakHere = std::max(peak1, peak2);

	if (false && peakHere >= maxPeakFromZero) {
		D_PRINTLN("peak:  %d  but max:  %d", peakHere, maxPeakFromZero);
	}

	uint32_t peak16 = ((int64_t)peakHere << 16) / maxPeakFromZero;

	return std::min<int32_t>(peak16 >> 8, 256); // Max 256 - for now. Looks great and bright.
	                                            // Must manually limit this, cos if we've ended up with values higher
	                                            // than our maxPeakFromZero, there'd be trouble otherwise
}

void WaveformRenderer::renderOneColForCollapseAnimation(int32_t xDisplayWaveform, int32_t xDisplayOutput,
                                                        int32_t maxPeakFromZero, int32_t progress,
                                                        RGB thisImage[][kDisplayWidth + kSideBarWidth],
                                                        WaveformRenderData* data, std::optional<RGB> rgb, bool reversed,
                                                        int32_t valueCentrePoint, int32_t valueSpan) {

	int32_t xDisplayData = xDisplayWaveform;
	if (reversed) {
		xDisplayData = kDisplayWidth - 1 - xDisplayData;
	}

	if (data->colStatus[xDisplayData] != COL_STATUS_INVESTIGATED) {
		return;
	}

	int32_t min24, max24;
	getColBarPositions(xDisplayData, data, &min24, &max24, valueCentrePoint, valueSpan);

	int32_t singleSquareBrightness = getColBrightnessForSingleRow(xDisplayData, maxPeakFromZero, data);

	renderOneColForCollapseAnimationInterpolation(xDisplayOutput, min24, max24, singleSquareBrightness, progress,
	                                              thisImage, rgb);
}

// For the explode animation. Crams multiple cols of source material into one col of output material.
// Crudely grabs the max values from all cols in range, which looks totally fine.
void WaveformRenderer::renderOneColForCollapseAnimationZoomedOut(
    int32_t xDisplayWaveformLeftEdge, int32_t xDisplayWaveformRightEdge, int32_t xDisplayOutput,
    int32_t maxPeakFromZero, int32_t progress, RGB thisImage[][kDisplayWidth + kSideBarWidth], WaveformRenderData* data,
    std::optional<RGB> rgb, bool reversed, int32_t valueCentrePoint, int32_t valueSpan) {

	int32_t xDisplayDataLeftEdge = xDisplayWaveformLeftEdge;
	int32_t xDisplayDataRightEdge = xDisplayWaveformRightEdge;
	if (reversed) {
		xDisplayDataLeftEdge = kDisplayWidth - 1 - xDisplayWaveformRightEdge;
		xDisplayDataRightEdge = kDisplayWidth - 1 - xDisplayWaveformLeftEdge;
	}

	int32_t min24Total = 2147483647;
	int32_t max24Total = -2147483648;

	int32_t singleSquareBrightnessTotal = 0;

	for (int32_t xDisplayDataNow = xDisplayDataLeftEdge; xDisplayDataNow <= xDisplayDataRightEdge; xDisplayDataNow++) {
		if (data->colStatus[xDisplayDataNow] != COL_STATUS_INVESTIGATED) {
			return;
		}

		int32_t min24, max24;
		getColBarPositions(xDisplayDataNow, data, &min24, &max24, valueCentrePoint, valueSpan);

		if (min24 < min24Total) {
			min24Total = min24;
		}
		if (max24 > max24Total) {
			max24Total = max24;
		}

		int32_t singleSquareBrightness = getColBrightnessForSingleRow(xDisplayDataNow, maxPeakFromZero, data);

		if (singleSquareBrightness > singleSquareBrightnessTotal) {
			singleSquareBrightnessTotal = singleSquareBrightness;
		}
	}

	renderOneColForCollapseAnimationInterpolation(xDisplayOutput, min24Total, max24Total, singleSquareBrightnessTotal,
	                                              progress, thisImage, rgb);
}

// Once we've derived the appropriate data from the waveform for one final col of pads,
// this does the vertical animation according to our current amount of expandedness
void WaveformRenderer::renderOneColForCollapseAnimationInterpolation(int32_t xDisplayOutput, int32_t min24,
                                                                     int32_t max24, int32_t singleSquareBrightness,
                                                                     int32_t progress,
                                                                     RGB thisImage[][kDisplayWidth + kSideBarWidth],
                                                                     std::optional<RGB> rgb) {

	int32_t minStart = ((int32_t)collapseAnimationToWhichRow - (kDisplayHeight >> 1)) << 24;
	int32_t maxStart = ((int32_t)collapseAnimationToWhichRow - (kDisplayHeight >> 1) + 1) << 24;

	int32_t minDistance = min24 - minStart;
	int32_t maxDistance = max24 - maxStart;
	int32_t brightnessDistance = 256 - singleSquareBrightness;

	int32_t minCurrent = minStart + (((int64_t)minDistance * progress) >> 16);
	int32_t maxCurrent = maxStart + (((int64_t)maxDistance * progress) >> 16);
	int32_t brightnessCurrent = singleSquareBrightness + ((brightnessDistance * progress) >> 16);

	drawColBar(xDisplayOutput, minCurrent, maxCurrent, thisImage, brightnessCurrent, rgb);
}

// Returns false if had trouble loading some (will often not be all) Clusters, e.g. cos we're in the card routine
bool WaveformRenderer::findPeaksPerCol(Sample* sample, int64_t xScrollSamples, uint64_t xZoomSamples,
                                       WaveformRenderData* data, SampleRecorder* recorder, int32_t xStart,
                                       int32_t xEnd) {
	if (xStart < 0 || xStart > xEnd || xEnd > kDisplayWidth) {
		return false;
	}

	if (xScrollSamples != data->xScroll || xZoomSamples != data->xZoom) {
		memset(data->colStatus, 0, sizeof(data->colStatus));
	}

	data->xScroll = xScrollSamples;
	data->xZoom = xZoomSamples;

	uint64_t numValidSamples;
	int32_t endClusters;
	if (recorder) {
		numValidSamples = recorder->numSamplesCaptured;
		endClusters = sample->clusters.getNumElements();
	}
	else {
		numValidSamples = sample->lengthInSamples;
		endClusters = sample->getFirstClusterIndexWithNoAudioData();
	}

	const uint32_t bytesPerSampleFrame = sample->byteDepth * sample->numChannels;
	uint64_t numValidBytes = sample->audioDataLengthBytes;
	if (recorder) {
		if (bytesPerSampleFrame == 0 || numValidSamples > std::numeric_limits<uint64_t>::max() / bytesPerSampleFrame) {
			return false;
		}
		numValidBytes = numValidSamples * bytesPerSampleFrame;
	}
	if (numValidBytes > std::numeric_limits<uint64_t>::max() - sample->audioDataStartPosBytes) {
		return false;
	}
	const uint64_t validAudioEndByte = numValidBytes + sample->audioDataStartPosBytes;

	bool hadAnyTroubleLoading = false;

	for (int32_t col = xStart; col < xEnd; col++) {

		if (data->colStatus[col] == COL_STATUS_INVESTIGATED) {
			continue;
		}

		data->colStatus[col] = COL_STATUS_INVESTIGATED; // Default, which we may override below

		if (xZoomSamples > std::numeric_limits<uint64_t>::max() / static_cast<uint64_t>(col + 1)) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}
		const uint64_t startOffset = static_cast<uint64_t>(col) * xZoomSamples;
		const uint64_t endOffset = static_cast<uint64_t>(col + 1) * xZoomSamples;
		const auto rawStart = deluge::gui::waveform::detail::addSampleOffset(xScrollSamples, startOffset);
		const auto rawEnd = deluge::gui::waveform::detail::addSampleOffset(xScrollSamples, endOffset);
		if (!rawStart.valid || !rawEnd.valid) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}

		int64_t colStartSample = rawStart.value;
		if (colStartSample >= 0 && static_cast<uint64_t>(colStartSample) >= numValidSamples) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}
		else if (colStartSample < 0) {
			colStartSample = 0;
		}

		int64_t colEndSample = rawEnd.value;

		// If this column extends further right than the end of the waveform...
		if (colEndSample >= 0 && static_cast<uint64_t>(colEndSample) >= numValidSamples) {

			// If we're still recording, we'll just want to come back and render this one when the waveform has grown to
			// cover this whole column
			if (recorder) {
				data->colStatus[col] = 0;
				continue;
			}
			colEndSample = static_cast<int64_t>(numValidSamples);
		}
		else if (colEndSample < 0) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}

		const auto colStartByte = deluge::gui::waveform::waveformSampleBytePosition(colStartSample, bytesPerSampleFrame,
		                                                                            sample->audioDataStartPosBytes);
		const auto colEndByte = deluge::gui::waveform::waveformSampleBytePosition(colEndSample, bytesPerSampleFrame,
		                                                                          sample->audioDataStartPosBytes);
		if (!colStartByte.valid || !colEndByte.valid) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}

		const auto colStartCluster =
		    deluge::gui::waveform::waveformClusterPosition(colStartByte.absoluteByte, Cluster::size_magnitude);
		const auto colEndCluster =
		    deluge::gui::waveform::waveformClusterPosition(colEndByte.absoluteByte, Cluster::size_magnitude);
		if (!colStartCluster.valid || !colEndCluster.valid) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}

		uint64_t clusterIndexToDoWide;
		int32_t startByteWithinCluster;
		int32_t endByteWithinCluster;

		const uint64_t numClustersSpan = colEndCluster.clusterIndex - colStartCluster.clusterIndex;

		bool investigatingAWholeCluster = false;

		// If both same cluster...
		if (numClustersSpan == 0) {
			clusterIndexToDoWide = colStartCluster.clusterIndex;
			startByteWithinCluster = static_cast<int32_t>(colStartCluster.byteWithinCluster);
			endByteWithinCluster = static_cast<int32_t>(colEndCluster.byteWithinCluster);
		}

		// Special case to make sure we get initial transient (we know there's more than 1 cluster)
		else if (colStartSample == 0 && colStartByte.absoluteByte < (Cluster::size >> 1)) {
			clusterIndexToDoWide = colStartCluster.clusterIndex;
			startByteWithinCluster = static_cast<int32_t>(colStartCluster.byteWithinCluster);
			endByteWithinCluster = Cluster::size;
			investigatingAWholeCluster = true;
		}

		// If 3 or more clusters, take 2nd one. TODO: have it take any one which has previously been fully investigated?
		else if (numClustersSpan >= 2) {
			clusterIndexToDoWide = colStartCluster.clusterIndex + 1;

			int32_t startByteWithinFirstCluster = static_cast<int32_t>(colStartCluster.byteWithinCluster);

			int32_t unusedBytesAtEndOfPrevCluster =
			    (Cluster::size - startByteWithinFirstCluster) % (sample->numChannels * sample->byteDepth);
			if (unusedBytesAtEndOfPrevCluster == 0) {
				startByteWithinCluster = 0;
			}
			else {
				startByteWithinCluster = (sample->numChannels * sample->byteDepth) - unusedBytesAtEndOfPrevCluster;
			}

			endByteWithinCluster = Cluster::size;
			investigatingAWholeCluster = true;
		}

		// If 2 cluster..
		else if (numClustersSpan == 1) {

			int32_t startByteWithinFirstCluster = static_cast<int32_t>(colStartCluster.byteWithinCluster);
			int32_t bytesInFirstCluster = Cluster::size - startByteWithinFirstCluster;

			int32_t bytesInSecondCluster = static_cast<int32_t>(colEndCluster.byteWithinCluster);

			// If more in first cluster...
			if (bytesInFirstCluster >= bytesInSecondCluster) {
				clusterIndexToDoWide = colStartCluster.clusterIndex;
				startByteWithinCluster = startByteWithinFirstCluster;
				endByteWithinCluster = Cluster::size;
			}

			// Or if more in second cluster...
			else {
				clusterIndexToDoWide = colEndCluster.clusterIndex;

				int32_t unusedBytesAtEndOfPrevCluster =
				    (Cluster::size - startByteWithinFirstCluster) % (sample->numChannels * sample->byteDepth);
				if (unusedBytesAtEndOfPrevCluster == 0) {
					startByteWithinCluster = 0;
				}
				else {
					startByteWithinCluster = (sample->numChannels * sample->byteDepth) - unusedBytesAtEndOfPrevCluster;
				}

				endByteWithinCluster = bytesInSecondCluster;
			}
		}

		if (endClusters <= 0 || clusterIndexToDoWide >= static_cast<uint64_t>(endClusters)
		    || clusterIndexToDoWide > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
			data->colStatus[col] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			continue;
		}
		const int32_t clusterIndexToDo = static_cast<int32_t>(clusterIndexToDoWide);

		if (clusterIndexToDo == endClusters - 1) {

			int32_t limit = static_cast<int32_t>(validAudioEndByte & (Cluster::size - 1));

			if (endByteWithinCluster > limit) {
				endByteWithinCluster = limit;
			}
		}

		SampleCluster* sampleCluster = sample->clusters.getElement(clusterIndexToDo);

		if (sampleCluster->cluster && sampleCluster->cluster->numReasonsToBeLoaded < 0) {
			FREEZE_WITH_ERROR("E449"); // Trying to catch errer before i028, which users have gotten.
		}

		// If we're wanting to investigate the whole length of one Cluster, and that's already actually been done
		// previously, we can just reuse those findings!
		if (investigatingAWholeCluster && sampleCluster->investigatedWholeLength) {
			data->minPerCol[col] = (int32_t)sampleCluster->minValue << 24;
			data->maxPerCol[col] = (int32_t)sampleCluster->maxValue << 24;
		}

		// Otherwise, do our normal investigation
		else {
			char const* errorCode;
			if (sampleCluster->cluster) {
				if (sampleCluster->cluster->loaded) {
					errorCode = "E343";
				}
				else {
					errorCode = "E344";
				}
			}
			else {
				errorCode = "E341"; // Qui got this, around V3.1.3! And Steven G, 3.1.5. And Brawny, V4.0.1-RC! And then
				                    // Malte P.
			}

			Cluster* cluster = sampleCluster->getCluster(sample, clusterIndexToDo, CLUSTER_LOAD_IMMEDIATELY);
			if (!cluster) {
cantReadData:
				D_PRINTLN("cant read");
				data->colStatus[col] = 0;
				hadAnyTroubleLoading = true;
				continue;
			}

			if (cluster->numReasonsToBeLoaded <= 0) {
				// Branko V got this. Trying to catch E340 below, which Ron R got while recording
				FREEZE_WITH_ERROR(errorCode);
			}

			uint32_t numBytesToRead = endByteWithinCluster - startByteWithinCluster;

			// Make the end-byte earlier, so we won't read past the end of the Cluster boundary
			int32_t overshoot = numBytesToRead % (sample->numChannels * sample->byteDepth);
			endByteWithinCluster -= overshoot;

			// However, if that's reduced us to 0 bytes to read, we know we're gonna have to load in the next Cluster to
			// get its sample that's on the boundary
			Cluster* nextCluster = nullptr;
			if (endByteWithinCluster <= startByteWithinCluster && clusterIndexToDo < endClusters - 1) {
				endByteWithinCluster += overshoot;
				SampleCluster* nextSampleCluster = sample->clusters.getElement(clusterIndexToDo + 1);
				if ((nextSampleCluster->cluster != nullptr) && nextSampleCluster->cluster->numReasonsToBeLoaded < 0) {
					FREEZE_WITH_ERROR("E450"); // Trying to catch errer before i028, which users have gotten.
				}
				nextCluster = nextSampleCluster->getCluster(sample, clusterIndexToDo + 1, CLUSTER_LOAD_IMMEDIATELY);

				if (cluster->numReasonsToBeLoaded <= 0) {
					FREEZE_WITH_ERROR("E342"); // Trying to catch E340 below, which Ron R got while recording
				}

				if (nextCluster == nullptr) {
					audioFileManager.removeReasonFromCluster(*cluster, "po8w");
					goto cantReadData;
				}

				// This entire block doesn't seem to actually do anything relevant - did I leave something out?
				// Shouldn't it have moved startByteWithinCluster etc into nextCluster? Rohan
			}

			numBytesToRead = endByteWithinCluster - startByteWithinCluster;

			// NOTE: from here on, we read *both* channels (if there are two), counting each one as a "sample"

			int32_t numSamplesToRead = numBytesToRead / sample->byteDepth;
			int32_t byteIncrement = sample->byteDepth;

			// We don't want to read endless samples. If we were gonna read lost, skip some.
			int32_t timesTooManySamples = ((numSamplesToRead - 1) >> SAMPLES_TO_READ_PER_COL_MAGNITUDE) + 1;
			if (timesTooManySamples > 1) {

				// If stereo sample, force an odd number here so we alternate between reading both channels
				if (sample->numChannels == 2) {
					if (!(timesTooManySamples & 1)) {
						timesTooManySamples++;
					}
				}

				byteIncrement *= timesTooManySamples;
			}

			// Misalign, to align with non-32-bit data
			startByteWithinCluster += sample->byteDepth - 4;
			endByteWithinCluster += sample->byteDepth - 4;

			int32_t bytePos = startByteWithinCluster;

			int32_t minThisCol = 2147483647;
			int32_t maxThisCol = -2147483648;

			// Go through the actual waveform of this cluster
			while (bytePos < endByteWithinCluster) {

				int32_t individualSampleValue =
				    *(int32_t*)&cluster->data[bytePos]; // & sample->bitMask; // bitMask hardly matters here

				if (individualSampleValue > maxThisCol) {
					maxThisCol = individualSampleValue;
				}
				if (individualSampleValue < minThisCol) {
					minThisCol = individualSampleValue;
				}

				bytePos += byteIncrement;
			}

			// If we just looked at the length of one entire cluster...
			if (investigatingAWholeCluster) {

				// See if we want to include any previously captured maximums and minimums, which might have looked at
				// slightly different values
				int32_t prevMin = (int32_t)sampleCluster->minValue << 24;
				int32_t prevMax = (int32_t)sampleCluster->maxValue << 24;

				if (prevMin < minThisCol) {
					minThisCol = prevMin;
				}
				if (prevMax > maxThisCol) {
					maxThisCol = prevMax;
				}

				// And mark the SampleCluster as fully investigated
				sampleCluster->minValue = minThisCol >> 24;
				sampleCluster->maxValue = maxThisCol >> 24;

				// Make rounding be towards 0
				if (sampleCluster->minValue < 0) {
					sampleCluster->minValue++;
				}
				if (sampleCluster->maxValue < 0) {
					sampleCluster->maxValue++;
				}

				sampleCluster->investigatedWholeLength = true;
			}

			// Or, if we only looked at a smaller part of a cluster...
			else {

				// Then just contribute to the running record of max and min found
				int8_t smallMin = minThisCol >> 24;
				int8_t smallMax = maxThisCol >> 24;

				// Make rounding be towards 0
				if (smallMin < 0) {
					smallMin++;
				}
				if (smallMax < 0) {
					smallMax++;
				}

				if (smallMin < sampleCluster->minValue) {
					sampleCluster->minValue = smallMin;
				}
				if (smallMax > sampleCluster->maxValue) {
					sampleCluster->maxValue = smallMax;
				}
			}

			data->maxPerCol[col] = maxThisCol;
			data->minPerCol[col] = minThisCol;

			audioFileManager.removeReasonFromCluster(*cluster, "E340"); // Ron R got this, when error was "iiuh"
			if (nextCluster != nullptr) {
				audioFileManager.removeReasonFromCluster(*nextCluster, "9700");
			}
			AudioEngine::routineWithClusterLoading();
		}
	}

	if (recorder != nullptr) {
		sample->maxValueFound = recorder->recordMax;
		sample->minValueFound = recorder->recordMin;
	}

	// Keep a running best for the max and min found for the whole Sample
	else {
		for (int32_t col = xStart; col < xEnd; col++) {
			if (data->colStatus[col] == COL_STATUS_INVESTIGATED) {
				if (data->maxPerCol[col] > sample->maxValueFound) {
					sample->maxValueFound = data->maxPerCol[col];
				}
				if (data->minPerCol[col] < sample->minValueFound) {
					sample->minValueFound = data->minPerCol[col];
				}
			}
		}
	}

	return !hadAnyTroubleLoading;
}

bool WaveformRenderer::findPeaksPerOledBucket(Sample* sample, int64_t xScroll, uint64_t xZoom,
                                              deluge::gui::waveform::OledWaveformRenderData* data) {
	using namespace deluge::gui::waveform;

	if (xZoom > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
		return false;
	}
	updateOledWaveformCacheViewport(*data, xScroll, static_cast<int64_t>(xZoom));

	WaveformRenderData scratch{};
	scratch.xScroll = -1;
	bool complete = true;
	WaveformSampleRange previousRange{};
	size_t previousBucket = 0;
	for (size_t bucket = 0; bucket < kOledWaveformBucketCount; bucket++) {
		if (data->colStatus[bucket] != 0) {
			previousRange = oledWaveformBucketSampleRange(xScroll, xZoom, bucket, kOledWaveformBucketCount);
			previousBucket = bucket;
			continue;
		}

		const WaveformSampleRange range =
		    oledWaveformBucketSampleRange(xScroll, xZoom, bucket, kOledWaveformBucketCount);
		if (!range.valid || range.end <= range.start) {
			data->colStatus[bucket] = COL_STATUS_INVESTIGATED_BUT_BEYOND_WAVEFORM;
			previousRange = range;
			previousBucket = bucket;
			continue;
		}

		if (bucket > 0 && previousRange.valid && range.start == previousRange.start && range.end == previousRange.end) {
			data->colStatus[bucket] = data->colStatus[previousBucket];
			if (data->colStatus[previousBucket] == COL_STATUS_INVESTIGATED) {
				data->minPerCol[bucket] = data->minPerCol[previousBucket];
				data->maxPerCol[bucket] = data->maxPerCol[previousBucket];
			}
			if (data->colStatus[bucket] == 0) {
				complete = false;
			}
			previousRange = range;
			previousBucket = bucket;
			continue;
		}

		scratch.xScroll = -1;
		const uint64_t bucketZoom = static_cast<uint64_t>(range.end - range.start);
		const bool loaded = findPeaksPerCol(sample, range.start, bucketZoom, &scratch, nullptr, 0, 1);
		data->colStatus[bucket] = scratch.colStatus[0];
		if (scratch.colStatus[0] == COL_STATUS_INVESTIGATED) {
			data->minPerCol[bucket] = scratch.minPerCol[0];
			data->maxPerCol[bucket] = scratch.maxPerCol[0];
		}
		if (!loaded || data->colStatus[bucket] == 0) {
			complete = false;
		}

		previousRange = range;
		previousBucket = bucket;
	}

	return complete;
}

void WaveformRenderer::getColBarPositions(int32_t xDisplay, WaveformRenderData* data, int32_t* min24, int32_t* max24,
                                          int32_t valueCentrePoint, int32_t valueSpan) {
	*min24 = ((int64_t)(data->minPerCol[xDisplay] - valueCentrePoint) << 24) / valueSpan;
	*max24 = ((int64_t)(data->maxPerCol[xDisplay] - valueCentrePoint) << 24) / valueSpan;

	// Ensure we're going to draw at least 1 pixel's width
	if (*max24 - *min24 < kMaxSampleValue) {
		int32_t midPoint = (*max24 >> 1) + (*min24 >> 1);
		*max24 = midPoint + 8388608;
		*min24 = midPoint - 8388608;
	}
}

void WaveformRenderer::drawColBar(int32_t xDisplay, int32_t min24, int32_t max24,
                                  RGB thisImage[][kDisplayWidth + kSideBarWidth], int32_t brightness,
                                  std::optional<RGB> rgb) {
	int32_t yStart = std::max((int32_t)(min24 >> 24), -(kDisplayHeight >> 1));
	int32_t yStop = std::min((int32_t)(max24 >> 24) + 1, kDisplayHeight >> 1);

	for (int32_t y = yStart; y < yStop; y++) {

		int32_t colourAmount; // Out of 256

		if (y == (min24 >> 24)) {
			int32_t howMuchThisSquare = (min24 - (y << 24)) >> 16; // Comes out as 8-bit
			colourAmount = brightness - ((howMuchThisSquare * brightness) >> 8);
		}

		else if (y < (max24 >> 24)) {
			colourAmount = brightness;
		}

		else {
			int32_t howMuchThisSquare = (max24 - (y << 24)) >> 16; // Comes out as 8-bit
			colourAmount = ((howMuchThisSquare * brightness) >> 8);
		}

		int32_t valueHere = (colourAmount * colourAmount) >> 8;
		RGB color = rgb.has_value()
		                ? rgb.value().transform([valueHere](auto channel) { return (valueHere * channel) >> 8; })
		                : RGB::monochrome(valueHere);

		thisImage[y + (kDisplayHeight >> 1)][xDisplay] = color;
	}
}

void WaveformRenderer::renderOneCol(Sample* sample, int32_t xDisplay, RGB thisImage[][kDisplayWidth + kSideBarWidth],
                                    WaveformRenderData* data, bool reversed, std::optional<RGB> rgb) {
	int32_t min24, max24;
	int32_t brightness = rgb ? 256 : 128;

	int32_t xDisplaySource = reversed ? (kDisplayWidth - 1 - xDisplay) : xDisplay;

	if (data->colStatus[xDisplaySource] == COL_STATUS_INVESTIGATED) {

		getColBarPositions(xDisplaySource, data, &min24, &max24, sample->getFoundValueCentrePoint(),
		                   sample->getValueSpan());

		drawColBar(xDisplay, min24, max24, thisImage, brightness, rgb);
	}
}
