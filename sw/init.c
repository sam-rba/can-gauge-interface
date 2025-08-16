#include <xc.h>

#include "init.h"

void
clockInit(void) {
	OSCCON = 0xFC; // HFINTOSC @ 16MHz, 3x PLL, PLL enabled
	ACTCON = 0x90; // active clock tuning enabled for USB
}

void
pinsInit(void) {
	// Disable all analog pin functions
	ANSELA = 0;
	ANSELB = 0;
	ANSELC = 0;
}
