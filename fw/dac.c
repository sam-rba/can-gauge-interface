#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

#include "dac.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

// Nominal reference voltage in millivolts
#define VREF_MV 5000

// Configuration bits:
// buffered, gain=1x, mode=active
typedef enum {
	CONFA = 0x7000, // DAC A
	CONFB = 0xF000, // DAC B
} Conf;

void
dacInit(void) {
	DAC1_CS_TRIS = OUT;
	DAC2_CS_TRIS = OUT;
	DAC1_CS = 1;
	DAC2_CS = 1;

	U16 level = 0u;
	dacSet1a(level);
	dacSet1b(level);
	dacSet2a(level);
	dacSet2b(level);
}

static void
set(U8 dacNum, Conf conf, U16 mv) {
	U16 level;

	mv = min(mv, VREF_MV); // clamp to 5V

	// D = 2^10 * Vout / Vref / (1000mV/V)
	level = (U16)((U32)mv * (1u<<10u) / VREF_MV);

	level <<= 2u; // D0 at bit 2
	level = ((U16)conf & 0xF000) | (level & 0x0FFF); // set config bits

	if (dacNum == 1u) {
		DAC1_CS = 0;
	} else {
		DAC2_CS = 0;
	}
	_delay(1);

	(void)spiTx((level>>8u) & 0xFF); // MSB
	(void)spiTx((level>>0u) & 0xFF); // LSB

	if (dacNum == 1u) {
		DAC1_CS = 1;
	} else {
		DAC2_CS = 1;
	}
}

void
dacSet1a(U16 mv) {
	set(1u, CONFA, mv);
}

void
dacSet1b(U16 mv) {
	set(1u, CONFB, mv);
}

void
dacSet2a(U16 mv) {
	set(2u, CONFA, mv);
}
void
dacSet2b(U16 mv) {
	set(2u, CONFB, mv);
}
