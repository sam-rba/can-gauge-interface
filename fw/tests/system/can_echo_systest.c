/* Listen for a frame with ID 123h or 1234567h;
 * echo it back with each byte of the DATA FIELD incremented by 1.
 */

#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "can.h"

static const CanId mask0 = {
	.isExt = true,
	.eid = 0x1FFFFFFF,
};
static const CanId filter0 = {
	.isExt = true,
	.eid = 0x1234567,
};
static const CanId mask1 = {
	.isExt = false,
	.sid = 0x7FF,
};
static const CanId filter2 = {
	.isExt = false,
	.sid = 0x123,
};

void
main(void) {
	sysInit();
	spiInit();
	canInit();

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING_10K);
	canSetMask0(&mask0); // RXB0
	canSetFilter0(&filter0);
	canSetMask1(&mask1); // RXB1
	canSetFilter2(&filter2);
	canIE(true); // enable interrupts on MCP2515's INT pin
	canSetMode(CAN_MODE_NORMAL);
	
	// Enable interrupts
	INTCON = 0x00; // clear flags
	OPTION_REGbits.INTEDG = 0; // interrupt on falling edge
	INTCONbits.INTE = 1; // enable INT pin
	INTCONbits.PEIE = 1; // enable peripheral interrupts
	INTCONbits.GIE = 1; // enable global interrupts

	for (;;) {

	}
}

// Add 1 to each data byte of a CAN frame and transmit it to the bus.
static void
echo(CanFrame *frame) {
	U8 k;

	for (k = 0u; k < frame->dlc; k++) {
		frame->data[k]++;
	}

	canTx(frame);
}

void
__interrupt() isr(void) {
	CanFrame frame;

	if (INTCONbits.INTF) {
		switch (canRxStatus() & 0x7) { // check filter match
		case 0u: // RXF0
			canReadRxb0(&frame);
			echo(&frame);
			break;
		case 2u: // RXF2
			canReadRxb1(&frame);
			echo(&frame);
			break;
		default:
			canReadRxb0(&frame); // clear interrupt flag
			canReadRxb1(&frame);
		}
		INTCONbits.INTF = 0;
	}
}
