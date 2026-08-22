/*
 * Copyright © 2026 Synthstrom Audible Limited
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

#include "model/consequence/consequence_note_row_reorder.h"

#include "model/clip/instrument_clip.h"

ConsequenceNoteRowReorder::ConsequenceNoteRowReorder(InstrumentClip* newClip, int32_t newFromIndex,
                                                     int32_t newToIndex)
    : clip(newClip), fromIndex(newFromIndex), toIndex(newToIndex) {}

Error ConsequenceNoteRowReorder::revert(TimeType time, ModelStack*) {
	int32_t fromIndexNow = fromIndex;
	int32_t toIndexNow = toIndex;

	if (time == BEFORE) {
		fromIndexNow = toIndex;
		toIndexNow = fromIndex;
	}

	if (fromIndexNow < 0 || toIndexNow < 0 || fromIndexNow >= clip->noteRows.getNumElements()
	    || toIndexNow >= clip->noteRows.getNumElements()) {
		return Error::BUG;
	}

	clip->noteRows.repositionElement(fromIndexNow, toIndexNow);
	return Error::NONE;
}
