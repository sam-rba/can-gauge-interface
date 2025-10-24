/* Listen for a standard DATA FRAME with ID 123h;
 * echo it back with each byte of the DATA FIELD incremented by 1.
 */

#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "can.h"

void
main(void) {
	sysInit();
	spiInit();
	canInit();

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING_10K);
	CanId id = {
		.type=CAN_ID_STD,
		.sid = {0xFF, 0xFF},
	};
	canSetMask0(&id); // set masks
	canSetMask1(&id);
	canSetFilter1(&id); // set unused filters
	canSetFilter2(&id);
	canSetFilter3(&id);
	canSetFilter4(&id);
	canSetFilter5(&id);
	id.sid = (U16){0x01, 0x23}; // listen for message on filter 0
	canSetFilter0(&id);
	canIE(true); // enable interrupts on INT pin
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
	U8 status;
	CanFrame frame;

	if (INTCONbits.INTF) {
		status = canRxStatus();
		switch (status & 0x7) { // check filter match
		case 0u: // RXF0
			canReadRxb0(&frame);
			echo(&frame);
			break;
		case 1u: // RXF1
			canReadRxb0(&frame); // clear interrupt flag
			break;
		default:
			// Message in RXB1
			canReadRxb1(&frame); // clear interrupt flag
		}

		INTCONbits.INTF = 0;
	}
}
