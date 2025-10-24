#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

#include "dac.h"

// Configuration bits:
// buffered, gain=1x, mode=active
typedef enum {
	CONFA = 0x70, // DAC A
	CONFB = 0xF0, // DAC B
} Conf;

void
dacInit(void) {
	DAC1_CS_TRIS = OUT;
	DAC2_CS_TRIS = OUT;
	DAC1_CS = 1;
	DAC2_CS = 1;

	U16 level = {0u, 0u};
	dacSet1a(level);
	dacSet1b(level);
	dacSet2a(level);
	dacSet2b(level);
}

static void
set(U8 dacNum, Conf conf, U16 level) {
	level = lshiftU16(level, 2u); // D0 at bit 2
	level.hi = (U8)conf | (level.hi & 0x0F); // set config bits

	if (dacNum == 1u) {
		DAC1_CS = 0;
	} else {
		DAC2_CS = 0;
	}
	_delay(1);

	(void)spiTx(level.hi);
	(void)spiTx(level.lo);

	if (dacNum == 1u) {
		DAC1_CS = 1;
	} else {
		DAC2_CS = 1;
	}
}

void
dacSet1a(U16 level) {
	set(1u, CONFA, level);
}

void
dacSet1b(U16 level) {
	set(1u, CONFB, level);
}

void
dacSet2a(U16 level) {
	set(2u, CONFA, level);
}
void
dacSet2b(U16 level) {
	set(2u, CONFB, level);
}
