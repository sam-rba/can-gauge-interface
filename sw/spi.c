#include <xc.h>

#include <stdint.h>

#include "sys.h"
#include "types.h"

#include "spi.h"

void
spiInit(void) {
	U8 junk;

	TRISBbits.TRISB4 = IN; // SDI
	TRISCbits.TRISC7 = OUT; // SDO
	TRISBbits.TRISB6 = OUT; // SCK

	SSPSTAT = 0x00;
	SSPCON1 = 0x01; // FOSC/16 => 3MHz SPI clock
	SSPCON1bits.SSPEN = 1; // enable
	junk = SSPBUF; // dummy read to clear BF
	(void)junk;
}

U8
spiTx(U8 c) {
	SSPBUF = c;
	while (!SSPSTATbits.BF) {}
	return SSPBUF;
}
