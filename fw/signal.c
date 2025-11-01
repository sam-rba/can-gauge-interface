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
sigPluck(const SigFmt *sig, const CanFrame *frame, Number *raw) {
	// Preconditions
	if (
		(sig->start >= (8u * frame->dlc)) // signal starts outside DATA FIELD
		|| (sig->size > (8u * frame->dlc - sig->start)) // signal extends outside DATA FIELD
		|| (sig->size < 1u) // signal is empty
	) {
		return FAIL;
	}

	// Read signal into U32
	switch (sig->order) {
	case LITTLE_ENDIAN:
		pluckLE(sig, frame, &raw->u32);
		break;
	case BIG_ENDIAN:
		pluckBE(sig, frame, &raw->u32);
		break;
	default:
		return FAIL; // invalid byte order
	}

	// Shrink signal to appropriate size and set +/- sign
	if (sig->size <= 8u) {
		raw->u8 = raw->u32 & 0xFF;
		if (sig->isSigned) {
			raw->type = NUM_I8;
			raw->i8 = *(I8 *)&raw->u8;
		} else {
			raw->type = NUM_U8;
		}
	} else if (sig->size <= 16u) {
		raw->u16 = raw->u32 & 0xFFFF;
		if (sig->isSigned) {
			raw->type = NUM_I16;
			raw->i16 = *(I16 *)&raw->u16;
		} else {
			raw->type = NUM_U16;
		}
	} else if (sig->size <= 32u) {
		if (sig->isSigned) {
			raw->type = NUM_I32;
			raw->i32 = *(I32 *)&raw->u32;
		} else {
			raw->type = NUM_U32;
		}
	} else {
		return FAIL; // signal too big
	}

	return OK;
}
