#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "can.h"

// Frame to transmit periodically
static const CanFrame frame = {
	.id = {
		.type = CAN_ID_STD,
		.sid = {0x01, 0x23},
	},
	.dlc = 8u,
	.data = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77},
	.rtr = false,
};

static U8 ctr = 0u; // timer

void
main(void) {
	sysInit();
	spiInit();
	canInit();

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING_10K);
	canSetMode(CAN_MODE_NORMAL);

	// Setup TMR1
	T1CON = 0x31; // Fosc/4, 1:8 prescaler
	PIE1bits.TMR1IE = 1; // enable interrupts
	PIR1bits.TMR1IF = 0; // clear flag

	// Enable interrupts
	INTCON = 0x00; // clear flags
	INTCONbits.PEIE = 1; // enable peripheral interrupts
	INTCONbits.GIE = 1; // enable global interrupts

	for (;;) {

	}
}

void
__interrupt() isr(void) {
	if (PIR1bits.TMR1IF) {
		if (++ctr == 114u) { // 5s period
			(void)canTx(&frame);
			ctr = 0u;
		}
		PIR1bits.TMR1IF = 0;
	}
}
