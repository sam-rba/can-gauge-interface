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
	.eid = 0x01272000, // 112720XXh
};

// ID Control filter.
// Used for writing/reading the CAN ID associated with each parameter.
// See `doc/datafmt.ps'.
static const CanId idCtrlFilter = {
	.isExt = true,
	.eid = 0x01272100, // 127210Xh
};

// Receive buffer 0 mask.
// RXB0 receives Table and ID Control Frames.
static const CanId rxb0Mask = {
	.isExt = true,
	.eid = 0x1FFFFF00, // all buf LSB
};

// Receive buffer 1 mask.
// RXB1 is used for receiving parameter values.
// The mask is permissive: all messages are accepted and filtered in software.
static const CanId rxb1Mask = {
	.isExt = true,
	.eid = 0u, // accept all messages
};

// Calibration tables in EEPROM:
// Each is 2*sizeof(U32)*TAB_ROWS = 256B -- too big for an 8-bit word.
// That's why the offsets are hard-coded.
static const Table tachTbl = {0ul*TAB_SIZE}; // tachometer
static const Table speedTbl = {1ul*TAB_SIZE}; // speedometer
static const Table an1Tbl = {2ul*TAB_SIZE}; // analog channels...
static const Table an2Tbl = {3ul*TAB_SIZE};
static const Table an3Tbl = {4ul*TAB_SIZE};
static const Table an4Tbl = {5ul*TAB_SIZE};

// EEPROM address of CAN ID for each parameter.
// Each of these addresses holds a U32 CAN ID.
static const EepromAddr paramIdAddrs[NPARAM] = {
	[TACH] = NPARAM*TAB_SIZE + 0ul*sizeof(U32), // tachometer
	[SPEED] = NPARAM*TAB_SIZE + 1ul*sizeof(U32), // speedometer
	[AN1] = NPARAM*TAB_SIZE + 2ul*sizeof(U32), // analog channels...
	[AN2] = NPARAM*TAB_SIZE + 3ul*sizeof(U32),
	[AN3] = NPARAM*TAB_SIZE + 4ul*sizeof(U32),
	[AN4] = NPARAM*TAB_SIZE + 5ul*sizeof(U32),
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
			INTCONbits.GIE = oldGie; // restore previous interrupt setting
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
		// RXB1 messages are filtered in software
	canIE(true); // enable interrupts on MCP2515's INT pin
	canSetMode(CAN_MODE_NORMAL);

	// Enable interrupts
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
	CanId paramId;
	CanFrame response;

	// Get parameter's CAN ID
	if ((U8)param < NPARAM) {
		paramId = paramIds[param];
	} else {
		return FAIL; // invalid parameter
	}

	// Pack param ID into response's DATA FIELD
	response.id = (CanId) {
		.isExt = true,
		.eid = 0x01272100 | ((U32)param & 0x0F), // 127210Xh
	};
	response.rtr = false;
	if (paramId.isExt) { // extended
		response.dlc = 4u; // U32
		response.data[0u] = (paramId.eid>>24u) & 0x1F;
		response.data[1u] = (paramId.eid>>16u) & 0xFF;
		response.data[2u] = (paramId.eid>>8u) & 0xFF;
		response.data[3u] = (paramId.eid>>0u) & 0xFF;
	} else { // standard
		response.dlc = 2u; // U16
		response.data[0u] = (paramId.sid>>8u) & 0x07;
		response.data[1u] = (paramId.sid>>0u) & 0xFF;
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
		paramId.eid = ((U32)frame->data[3u] << 0u)
			| ((U32)frame->data[2u] << 8u)
			| ((U32)frame->data[1u] << 16u)
			| (((U32)frame->data[0u] & 0x1F) << 24u);
	} else if (frame->dlc == 2u) { // standard
		paramId.isExt = false;
		paramId.sid = ((U16)frame->data[1u] << 0u)
			| ((U16)frame->data[0u] << 8u);
	} else {
		return FAIL; // invalid DLC
	}

	// Set param's ID
	param = frame->id.eid & 0xF;
	if ((U8)param < NPARAM) {
		// Update copy in EEPROM
		status = eepromWriteCanId(paramIdAddrs[param], &paramId);
		if (status != OK) {
			return FAIL; // write failed
		}
		// Update copy in RAM
		paramIds[param] = paramId;
	} else {
		return FAIL; // invalid parameter
	}

	// TODO: remove
	respondIdCtrl(param); // echo

	return OK;
}

// Handle an ID Control Frame.
// See `doc/datafmt.ps'
static Status
handleIdCtrlFrame(const CanFrame *frame) {
	Param param;

	if (frame->rtr) { // REMOTE
		param = frame->id.eid & 0xF;
		return respondIdCtrl(param); // respond with the parameter's CAN ID
	} else { // DATA
		return setParamId(frame);
	}
}

// TODO: remove
static void
echo(const CanFrame *frame) {
	CanFrame out;
	U8 k;

	memmove(&out, frame, sizeof(*frame));
	out.rtr = false;
	for (k = 0u; k < out.dlc; k++) {
		out.data[k]++;
	}
	canTx(&out);
}

// Handle a frame potentially holding a parameter value.
static Status
handleParamFrame(const CanFrame *frame) {
	// TODO
	echo(frame);
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
