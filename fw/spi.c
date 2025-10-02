#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"

#include "spi.h"

void
spiInit(void) {
	U8 junk;

	TRISBbits.TRISB4 = IN; // SDI
	TRISCbits.TRISC7 = OUT; // SDO
	TRISBbits.TRISB6 = OUT; // SCK

	SSPSTAT = 0x00;
	SSPCON1 = 0x22; // FOSC/64 => 750kHz SPI clock
	junk = SSPBUF; // dummy read to clear BF
	(void)junk;
}

U8
spiTx(U8 c) {
	SSPBUF = c;
	while (!SSPSTATbits.BF) {}
	return SSPBUF;
}
