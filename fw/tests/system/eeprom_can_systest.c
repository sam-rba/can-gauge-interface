/* Listen for a standard frame with ID 123h; read or write address Ah of the EEPROM.
 *
 * If the frame is a DATA FRAME, D0 is written to address Ah of the EEPROM.
 *
 * If the frame is a REMOTE FRAME, address Ah of the EEPROM is read and the value
 * is sent in the D0 field of a DATA FRAME with the same ID.
 */

#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "eeprom.h"
#include "can.h"

static const CanId id = {
	.type = CAN_ID_STD,
	.sid = {0x1, 0x23},
};
static const U16 addr = {0x00, 0x0A};

void
main(void) {
	sysInit();
	spiInit();
	eepromInit();
	canInit();

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING_10K);
	CanId mask = {
		.type=CAN_ID_STD,
		.sid = {0xFF, 0xFF},
	};
	canSetMask0(&mask); // set masks
	canSetMask1(&mask);
	canSetFilter1(&mask); // set unused filters
	canSetFilter2(&mask);
	canSetFilter3(&mask);
	canSetFilter4(&mask);
	canSetFilter5(&mask);
	canSetFilter0(&id); // listen for message on filter 0
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

// Read the EEPROM and transmit the value in a CAN frame.
static Status
readEeprom(const CanFrame *frame) {
	U8 val;
	Status status;

	status = eepromRead(addr, &val, 1u);
	if (status != OK) {
		return FAIL;
	}

	CanFrame out = {
		.id = id,
		.rtr = false,
		.dlc = 1u,
		.data = {val},
	};
	return canTx(&out);
}

// Write a value to the EEPROM.
static Status
writeEeprom(const CanFrame *frame) {
	U8 val;

	if (frame->dlc < 1u) {
		return FAIL; // malformed frame
	}

	val = frame->data[0u];
	return eepromWrite(addr, &val, 1u);
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
			if (frame.rtr) {
				(void)readEeprom(&frame);
			} else {
				(void)writeEeprom(&frame);
			}
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
