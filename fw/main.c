#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "dac.h"
#include "can.h"
#include "eeprom.h"
#include "table.h"

// TODO: auto baud detection
#define CAN_TIMING CAN_TIMING_10K

// Parameters
typedef enum {
	TACH = 0,
	SPEED,
	AN1,
	AN2,
	AN3,
	AN4,

	NPARAM,
} Param;

// Table Control filter.
// Used for writing/reading calibration tables.
static const CanId tblCtrlFilter = {
	.isExt = true,
	.eid = {0x00, 0x20, 0x27, 0x01}, // 12720XXh
};

// ID Control filter.
// Used for writing/reading the CAN ID associated with each parameter.
// See `doc/datafmt.ps'.
static const CanId idCtrlFilter = {
	.isExt = true,
	.eid = {0x00, 0x21, 0x27, 0x01}, // 127210Xh
};

// Receive buffer 0 mask.
// RXB0 is used for control messages for reading/writing calibration tables
// and CAN IDs for each parameter.
// This mask is the union of the two control message filters.
static const CanId rxb0Mask = {
	.isExt = true,
	.eid = {0x00, 0x21, 0x27, 0x01}, // 1272100h
};

// Receive buffer 1 mask.
// RXB1 is used for receiving parameter values.
// The mask is permissive: all messages are accepted and filtered in software.
static const CanId rxb1Mask = {
	.isExt = true,
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
static const EepromAddr paramIdAddrs[NPARAM] = {
	[TACH] = {0x06, 0x00}, // tachometer
	[SPEED] = {0x06, 0x04}, // speedometer
	[AN1] = {0x06, 0x08}, // analog channels...
	[AN2] = {0x06, 0x0C},
	[AN3] = {0x06, 0x10},
	[AN4] = {0x06, 0x14},
};

// CAN ID of each parameter
static volatile CanId paramIds[NPARAM];

// Load parameters' CAN IDs from EEPROM
static Status
loadParamIds(void) {
	U8 oldGie, k;
	Status status;

	// Disable interrupts so the volatile address pointers can be passed safely
	oldGie = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	for (k = 0u; k < NPARAM; k++) {
		status = eepromReadCanId(paramIdAddrs[k], (CanId*)&paramIds[k]);
		if (status != OK) {
			return FAIL;
		}
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

// Transmit the response to an ID Control REMOTE FRAME.
// The response is an ID Control DATA FRAME containing the CAN ID
// of the requested parameter.
static Status
respondIdCtrl(Param param) {
	CanFrame response;
	CanId paramId;
	Status status;

	// Read parameter's CAN ID from EEPROM
	if ((U8)param < NPARAM) {
		status = eepromReadCanId(paramIdAddrs[param], &paramId);
		if (status != OK) {
			return FAIL; // read failed
		}
	} else {
		return FAIL; // invalid parameter
	}

	// Pack param ID into response's DATA FIELD
	response.id = (CanId) {
		.isExt = true,
		.eid = {param & 0xF, 0x21, 0x27, 0x01}, // 127210Xh
	};
	response.rtr = false;
	if (paramId.isExt) { // extended
		response.dlc = 4u; // U32
		// BE <- LE
		response.data[0u] = paramId.eid[3u] & 0x1F;
		response.data[1u] = paramId.eid[2u];
		response.data[2u] = paramId.eid[1u];
		response.data[3u] = paramId.eid[0u];
	} else { // standard
		response.dlc = 2u; // U16
		response.data[0u] = paramId.sid.hi & 0x7;
		response.data[1u] = paramId.sid.lo;
	}

	// Transmit response
	return canTx(&response);
}

// Set the CAN ID associated with a parameter in response to an ID Control DATA FRAME.
static Status
setParamId(const CanFrame *frame) {
	CanId paramId;
	Param param;
	Status status;

	// Extract param ID from DATA FIELD
	if (frame->dlc == 4u) { // extended
		paramId.isExt = true;
		// LE <- BE
		paramId.eid[0u] = frame->data[3u];
		paramId.eid[1u] = frame->data[2u];
		paramId.eid[2u] = frame->data[1u];
		paramId.eid[3u] = frame->data[0u] & 0x1F;
	} else if (frame->dlc == 2u) { // standard
		paramId.isExt = false;
		paramId.sid.lo = frame->data[1u];
		paramId.sid.hi = frame->data[0u] & 0x7;
	} else {
		return FAIL; // invalid DLC
	}

	// Extract param # from Control Frame's ID
	param = frame->id.eid[0u] & 0xF;
	if ((U8)param >= NPARAM) {
		return FAIL; // invalid parameter
	}

	// Update copy in EEPROM
	status = eepromWriteCanId(paramIdAddrs[param], &paramId);
	if (status != OK) {
		return FAIL; // write failed
	}

	// Update copy in PIC's RAM
	paramIds[param] = paramId; 

	return OK;
}

// Handle an ID Control Frame.
// See `doc/datafmt.ps'
static Status
handleIdCtrlFrame(const CanFrame *frame) {
	Param param;

	if (frame->rtr) { // REMOTE
		param = frame->id.eid[0u] & 0xF;
		return respondIdCtrl(param); // respond with the parameter's CAN ID
	} else { // DATA
		return setParamId(frame);
	}
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
