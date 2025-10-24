#include <xc.h>

#include <stdint.h>

#include "types.h"

U16
addU16(U16 a, U16 b) {
	a.hi += b.hi;
	a.lo += b.lo;
	if (STATUSbits.C) {
		a.hi++;
	}
	return a;
}

U16
addU16U8(U16 a, U8 b) {
	a.lo += b;
	if (STATUSbits.C) {
		a.hi++;
	}
	return a;
}

U16
lshiftU16(U16 a, U8 b) {
	a.hi = (U8)(a.hi << b) | (a.lo >> (8u-b));
	a.lo <<= b;
	return a;
}

U16
rshiftU16(U16 a, U8 b) {
	a.lo = (U8)((a.hi >> (8u-b)) << (8u-b)) | (U8)(a.lo >> b);
	a.hi >>= b;
	return a;
}

I8
cmpU16(U16 a, U16 b) {
	if (a.hi > b.hi) {
		return 1;
	} else if (a.hi < b.hi) {
		return -1;
	} else if (a.lo > b.lo) {
		return 1;
	} else if (a.lo < b.lo) {
		return -1;
	} else {
		return 0;
	}
}
