#include <xc.h>

#include <stdint.h>

#include "types.h"

void
incU16(U16 *x) {
	x->lo++;
	if (x->lo == 0u) {
		x->hi++;
	}
}
