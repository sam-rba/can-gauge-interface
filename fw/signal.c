#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "can.h"

#include "signal.h"
#include "signal_utestable.h"

// Extract a little-endian value from a frame.
// Assumes signal is within the frame's DATA FIELD.
void
pluckLE(const SigFmt *sig, const CanFrame *frame, U32 *raw) {
	U8 i, end, mask, byte;

	*raw = 0ul;
	end = sig->start + sig->size;

	// First iteration starts at arbitrary bit: namely the (i%8)'th bit.
	// Subsequent iterations start at bit 0 of each byte.

	for (i = sig->start; i < end; i += 8u-(i%8u)) {
		mask = (U8)(0xFF << (i%8u));
		if (i/8u == end/8u) { // if end in this byte
			mask &= 0xFF >> (8u - (end%8u)); // ignore top bits
		}
		byte = (frame->data[i/8u] & mask) >> (i%8u);
		*raw |= (U32)byte << (i - sig->start);
	}
}

// Extract a big-endian value from a frame.
// Assumes signal is within the frame's DATA FIELD.
void
pluckBE(const SigFmt *sig, const CanFrame *frame, U32 *raw) {
	U8 i, end, mask;

	*raw = 0ul;
	end = sig->start + sig->size;

	// First iteration starts at arbitrary bit: namely the (i%8)'th bit.
	// Subsequent iterations start at bit 0 of each byte.

	for (i = sig->start; i < end; i += 8u-(i%8u)) {
		mask = (U8)(0xFF << (i%8u));
		if (i/8u == end/8u) { // if end in this byte
			mask &= 0xFF >> (8u - (end%8u)); // ignore top bits
			*raw <<= (end%8u) - (i%8u); // include bits between i and end
		} else {
			*raw <<= 8u - (i%8u); // include bits between i and end of byte
		}
		*raw |= (frame->data[i/8u] & mask) >> (i%8u);
	}
}

Status
sigPluck(const SigFmt *sig, const CanFrame *frame, I32 *raw) {
	U32 uraw;

	// Preconditions
	if (
		(sig->start >= (8u * frame->dlc)) // signal starts outside DATA FIELD
		|| (sig->size > (8u * frame->dlc - sig->start)) // signal extends outside DATA FIELD
		|| (sig->size < 1u) // signal is empty
	) {
		return FAIL;
	}

	switch (sig->order) {
	case LITTLE_ENDIAN:
		pluckLE(sig, frame, &uraw);
		break;
	case BIG_ENDIAN:
		pluckBE(sig, frame, &uraw);
		break;
	default:
		return FAIL; // invalid byte order
	}
	*raw = *(I32 *)&uraw;
	return OK;
}
