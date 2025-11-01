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

// Signals
typedef enum {
	SIG_TACH = 0,
	SIG_SPEED,
	SIG_AN1,
	SIG_AN2,
	SIG_AN3,
	SIG_AN4,

	NSIGNALS,
} Signal;

// Table Control filter.
// Used for writing/reading calibration tables.
static const CanId tblCtrlFilter = {
	.isExt = true,
	.eid = 0x01272000, // 112720XXh
};

// ID Control filter.
// Used for writing/reading the CAN ID associated with each signal.
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
// RXB1 is used for receiving signals.
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

// EEPROM address of CAN ID for each signal.
// Each of these addresses holds a U32 CAN ID.
static const EepromAddr sigIdAddrs[NSIGNALS] = {
	[SIG_TACH] = NSIGNALS*TAB_SIZE + 0ul*sizeof(U32), // tachometer
	[SIG_SPEED] = NSIGNALS*TAB_SIZE + 1ul*sizeof(U32), // speedometer
	[SIG_AN1] = NSIGNALS*TAB_SIZE + 2ul*sizeof(U32), // analog channels...
	[SIG_AN2] = NSIGNALS*TAB_SIZE + 3ul*sizeof(U32),
	[SIG_AN3] = NSIGNALS*TAB_SIZE + 4ul*sizeof(U32),
	[SIG_AN4] = NSIGNALS*TAB_SIZE + 5ul*sizeof(U32),
};

// CAN ID of each signal
static volatile CanId sigIds[NSIGNALS];

// Load signals' CAN IDs from EEPROM
static Status
loadSigIds(void) {
	U8 oldGie, k;
	Status status;

	// Disable interrupts so the volatile address pointers can be passed safely
	oldGie = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	for (k = 0u; k < NSIGNALS; k++) {
		status = eepromReadCanId(sigIdAddrs[k], (CanId*)&sigIds[k]);
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

	// Load signals' CAN IDs from EEPROM
	status = loadSigIds();
	if (status != OK) {
		reset();
	}

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING);
	canSetMask0(&rxb0Mask); // RXB0 receives control messages
	canSetFilter0(&tblCtrlFilter); // Table Control Frames
	canSetFilter1(&idCtrlFilter); // ID Control Frames
	canSetMask1(&rxb1Mask); // RXB1 receives signal values
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
// of the requested signal.
static Status
respondIdCtrl(Signal sig) {
	CanId sigId;
	CanFrame response;

	// Get signal's CAN ID
	if ((U8)sig < NSIGNALS) {
		sigId = sigIds[sig];
	} else {
		return FAIL; // invalid signal
	}

	// Pack signal's ID into response's DATA FIELD
	response.id = (CanId) {
		.isExt = true,
		.eid = 0x01272100 | (sig & 0x0F), // 127210Xh
	};
	response.rtr = false;
	if (sigId.isExt) { // extended
		response.dlc = 4u; // U32
		response.data[0u] = (sigId.eid>>24u) & 0x1F;
		response.data[1u] = (sigId.eid>>16u) & 0xFF;
		response.data[2u] = (sigId.eid>>8u) & 0xFF;
		response.data[3u] = (sigId.eid>>0u) & 0xFF;
	} else { // standard
		response.dlc = 2u; // U16
		response.data[0u] = (sigId.sid>>8u) & 0x07;
		response.data[1u] = (sigId.sid>>0u) & 0xFF;
	}

	// Transmit response
	return canTx(&response);
}

// Set the CAN ID associated with a signal in response to an ID Control DATA FRAME.
static Status
setSigId(const CanFrame *frame) {
	CanId sigId;
	Signal sig;
	Status status;

	// Extract signal ID from DATA FIELD
	if (frame->dlc == 4u) { // extended
		sigId.isExt = true;
		sigId.eid = ((U32)frame->data[3u] << 0u)
			| ((U32)frame->data[2u] << 8u)
			| ((U32)frame->data[1u] << 16u)
			| (((U32)frame->data[0u] & 0x1F) << 24u);
	} else if (frame->dlc == 2u) { // standard
		sigId.isExt = false;
		sigId.sid = ((U16)frame->data[1u] << 0u)
			| ((U16)frame->data[0u] << 8u);
	} else {
		return FAIL; // invalid DLC
	}

	// Set signal's ID
	sig = frame->id.eid & 0xF;
	if ((U8)sig < NSIGNALS) {
		// Update copy in EEPROM
		status = eepromWriteCanId(sigIdAddrs[sig], &sigId);
		if (status != OK) {
			return FAIL; // write failed
		}
		// Update copy in RAM
		sigIds[sig] = sigId;
	} else {
		return FAIL; // invalid signal
	}

	// TODO: remove
	respondIdCtrl(sig); // echo

	return OK;
}

// Handle an ID Control Frame.
// See `doc/datafmt.ps'
static Status
handleIdCtrlFrame(const CanFrame *frame) {
	Signal sig;

	if (frame->rtr) { // REMOTE
		sig = frame->id.eid & 0xF;
		return respondIdCtrl(sig); // respond with the signal's CAN ID
	} else { // DATA
		return setSigId(frame);
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

// Handle a frame potentially holding a signal value.
static Status
handleSigFrame(const CanFrame *frame) {
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
		case 1u: // RXF1: signal ID control
			canReadRxb0(&frame);
			(void)handleIdCtrlFrame(&frame);
			break;
		default: // message in RXB1
			canReadRxb1(&frame);
			(void)handleSigFrame(&frame);
		}
		INTCONbits.INTF = 0; // clear flag
	}
}
