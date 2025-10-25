#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "dac.h"
#include "can.h"
#include "eeprom.h"
#include "table.h"

// TODO: auto baud detection
#define CAN_TIMING CAN_TIMING_10K

// Table Control filter.
// Used for writing/reading calibration tables.
static const CanId tblCtrlFilter = {
	.type = CAN_ID_EXT,
	.eid = {0x00, 0x20, 0x27, 0x01}, // 12720XXh
};

// ID Control filter.
// Used for writing/reading the CAN ID associated with each parameter.
// See `doc/datafmt.ps'.
static const CanId idCtrlFilter = {
	.type = CAN_ID_EXT,
	.eid = {0x00, 0x21, 0x27, 0x01}, // 127210Xh
};

// Receive buffer 0 mask.
// RXB0 is used for control messages for reading/writing calibration tables
// and CAN IDs for each parameter.
// This mask is the union of the two control message filters.
static const CanId rxb0Mask = {
	.type = CAN_ID_EXT,
	.eid = {0x00, 0x21, 0x27, 0x01}, // 1272100h
};

// Receive buffer 1 mask.
// RXB1 is used for receiving parameter values.
// The mask is permissive: all messages are accepted and filtered in software.
static const CanId rxb1Mask = {
	.type = CAN_ID_EXT,
	.eid = {0u, 0u, 0u, 0u}, // accept all messages
};

// Calibration tables in EEPROM:
// Each is 2*sizeof(U32)*TAB_ROWS = 256B -- too big for an 8-bit word.
// That's why the offsets are hard-coded.
static const Table tachTbl = {{0x00, 0x00}}; // tachometer
static const Table speedTbl = {{0x01, 0x00}}; // speedometer
static const Table an1Tbl = {{0x02, 0x00}}; // analog channels...
static const Table an2Tbl = {{0x03, 0x00}};
static const Table an3Tbl = {{0x04, 0x00}};
static const Table an4Tbl = {{0x05, 0x00}};

// EEPROM address of CAN ID for each parameter.
// Each of these addresses holds a U32 CAN ID.
static const EepromAddr tachIdAddr = {0x06, 0x00}; // tachometer
static const EepromAddr speedIdAddr = {0x06, 0x04}; // speedometer
static const EepromAddr an1IdAddr = {0x06, 0x08}; // analog channels...
static const EepromAddr an2IdAddr = {0x06, 0x0C};
static const EepromAddr an3IdAddr = {0x06, 0x10};
static const EepromAddr an4IdAddr = {0x06, 0x14};

static volatile CanId tachId; // tachometer
static volatile CanId speedId; // speedometer
static volatile CanId an1Id; // analog channels...
static volatile CanId an2Id;
static volatile CanId an3Id;
static volatile CanId an4Id;

// Load parameters' CAN IDs from EEPROM
static Status
loadParamIds(void) {
	U8 oldGie;
	Status status;

	// Disable interrupts so the volatile address pointers can be passed safely
	oldGie = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	status = eepromReadCanId(tachIdAddr, (CanId*)&tachId);
	if (status != OK) {
		return FAIL;
	}
	status = eepromReadCanId(speedIdAddr, (CanId*)&speedId);
	if (status != OK) {
		return FAIL;
	}
	status = eepromReadCanId(an1IdAddr, (CanId*)&an1Id);
	if (status != OK) {
		return FAIL;
	}
	status = eepromReadCanId(an2IdAddr, (CanId*)&an2Id);
	if (status != OK) {
		return FAIL;
	}
	status = eepromReadCanId(an3IdAddr, (CanId*)&an3Id);
	if (status != OK) {
		return FAIL;
	}
	status = eepromReadCanId(an4IdAddr, (CanId*)&an4Id);
	if (status != OK) {
		return FAIL;
	}

	// Restore previous interrupt setting
	INTCONbits.GIE = oldGie;

	return OK;
}

static void
reset(void) {
	_delay(100000);
	asm("RESET");
}

void
main(void) {
	Status status;

	sysInit();
	spiInit();
	dacInit();
	canInit();
	eepromInit();

	// Load parameters' CAN IDs from EEPROM
	status = loadParamIds();
	if (status != OK) {
		reset();
	}

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING);
	canSetMask0(&rxb0Mask); // RXB0 receives control messages
	canSetFilter0(&tblCtrlFilter); // Table Control Frames
	canSetFilter1(&idCtrlFilter); // ID Control Frames
	canSetMask1(&rxb1Mask); // RXB1 receives parameter values
	// Permissive RXB1 mask; filter frames in software

	// Enable interrupts
	canIE(true); // enable interrupts on MCP2515's INT pin
	INTCON = 0x00; // clear flags
	OPTION_REGbits.INTEDG = 0; // interrupt on falling edge of INT pin
	INTCONbits.INTE = 1; // enable INT pin
	INTCONbits.PEIE = 1; // enable peripheral interrupts
	INTCONbits.GIE = 1; // enable global interrupts

	for (;;) {

	}
}

// Handle a Table Control Frame.
// See `doc/datafmt.ps'
static Status
handleTblCtrlFrame(const CanFrame *frame) {
	// TODO
}

// Handle an ID Control Frame.
// See `doc/datafmt.ps'
static Status
handleIdCtrlFrame(const CanFrame *frame) {
	// TODO
}

// Handle a frame potentially holding a parameter value.
static Status
handleParamFrame(const CanFrame *frame) {
	// TODO
}

void
__interrupt() isr(void) {
	U8 rxStatus;
	CanFrame frame;

	if (INTCONbits.INTF) { // CAN interrupt
		rxStatus = canRxStatus();
		switch (rxStatus & 0x7) { // check filter hit
		case 0u: // RXF0: calibration table control
			canReadRxb0(&frame);
			(void)handleTblCtrlFrame(&frame);
			break;
		case 1u: // RXF1: parameter ID control
			canReadRxb0(&frame);
			(void)handleIdCtrlFrame(&frame);
			break;
		default: // message in RXB1
			canReadRxb1(&frame);
			(void)handleParamFrame(&frame);
		}
		INTCONbits.INTF = 0; // clear flag
	}
}
