#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"

#include "spi.h"

void
spiInit(void) {
	U8 junk;

	TRISB4 = IN; // SDI
	TRISC7 = OUT; // SDO
	TRISB6 = OUT; // SCK

	SSPSTAT = 0x40; // CKE=1
	SSPCON1 = 0x21; // FOSC/16 => 3MHz SPI clock
	junk = SSPBUF; // dummy read to clear BF
	(void)junk;
}

U8
spiTx(U8 c) {
	SSPBUF = c;
	while (!SSPSTATbits.BF) {}
	return SSPBUF;
}
