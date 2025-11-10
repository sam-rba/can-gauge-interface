#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"

// (pulse/min) = 60 * (Fosc/4) / ((pre)*(post)*(PR2+1)*(period))
// = 60 * (48e6/4) / (64*16*10*(period))
// = 70313 / (period)
#define SPEED_FACTOR 70313ul
#define MIN_SPEED_PULSE_PER_MIN 2u // (pulse/min) >= SPEED_FACTOR / period_max)
#define EDGE_PER_PULSE 2u // rising and falling edge

static volatile U16 tmr2Period = 0u;

void
main(void) {
	sysInit();

	U16 pulsePerMin = 550u;
	tmr2Period = ((U32)SPEED_FACTOR / pulsePerMin) & 0xFFFF;

	T2CON = 0x7F; // postscaler=1:16, enable=1, prescaler=1:64
	PR2 = 10u-1u; // period = PR2+1

	INTCON = 0x00;
	PIE1bits.TMR2IE = 1;
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;

	for (;;) {

	}
}

void __interrupt()
isr(void) {
	static U16 tmr2Ctr = 0u;

	if (TMR2IF) { // speedometer
		if (++tmr2Ctr >= (tmr2Period/EDGE_PER_PULSE)) {
			tmr2Ctr = 0u;
			SPEED_PIN ^= 1; // toggle speedometer output
		}
		TMR2IF = 0;
	}
}
