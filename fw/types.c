#include <xc.h>

#include <stdint.h>

#include "types.h"

void
addU16(U16 *a, U8 b) {
	a->lo += b;
	if (STATUSbits.C) {
		a->hi++;
	}
}

void
lshiftU16(U16 *a, U8 b) {
	a->hi = (U8)(a->hi << b) | (a->lo >> (8u-b));
	a->lo <<= b;
}
