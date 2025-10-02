#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "eeprom.h"
#include "usb.h"

void
main(void) {
	sysInit();
	spiInit();
	eepromInit();
	usbInit();

	T1CON = 0;
	T1CONbits.TMR1CS = 0x0; // FOSC/4
	T1CONbits.T1CKPS = 0x3; // 1:8 prescaler
	T1CONbits.TMR1ON = 1;

	TMR1IE = 1;
	TMR1IF = 0;
	PEIE = 1;
	GIE = 1;

	for (;;) {

	}
}

void
__interrupt() isr(void) {
	static U8 ctr = 0u;

	if (TMR1IF) {
		if (ctr == 23u) { // 1s period

		}
		ctr = (ctr+1u) % 23u;
		TMR1IF = 0;
	}
}
