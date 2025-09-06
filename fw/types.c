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
