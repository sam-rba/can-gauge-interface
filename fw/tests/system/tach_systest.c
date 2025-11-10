#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"

// (pulse/min) = 60 * (Fosc/4) / ((pre)*(post)*(2^16 - TMR1))
//  = 60 * (48e6/4) / (8*6*(2^16 - TMR1))
//  = 15000000 / (2^16 - TMR1)
#define TACH_FACTOR 15000000ul
#define TMR1_POST 6u // TMR1 postscaler -- must be multiple of 2
#define EDGE_PER_PULSE 2u // rising and falling edge

static volatile U16 tmr1Start = 0u;

void
main(void) {
	sysInit();

	U16 pulsePerMin = 2500u;
	tmr1Start = (((U32)1ul<<16u) - (U32)TACH_FACTOR / (U32)pulsePerMin) & 0xFFFF;

	T1CON = 0x31; // source=Fosc/4, prescaler=1:8, enable=1

	INTCON = 0x00;
	PIE1bits.TMR1IE = 1;
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;

	for (;;) {

	}
}

void __interrupt()
isr(void) {
	static U8 tmr1Ctr = 0u;

	if (TMR1IF) { // tachometer
		if (++tmr1Ctr == (TMR1_POST/EDGE_PER_PULSE)) {
			tmr1Ctr = 0u;
			TACH_PIN ^= 1; // toggle tachometer output
		}
		TMR1H = (tmr1Start>>8u)&0xFF; // reset timer
		TMR1L = (tmr1Start>>0u)&0xFF;
		TMR1IF = 0;
	}
}
