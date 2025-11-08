#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "eeprom.h"
#include "dac.h"
#include "can.h"
#include "signal.h"
#include "serial.h"
#include "table.h"

// TODO: auto baud detection
#define CAN_TIMING CAN_TIMING_10K

#define TAB_CTRL_CAN_ID 0x1272000 // Table Control Frame ID
#define SIG_CTRL_CAN_ID 0x1272100 // Signal Control Frame ID
#define ERR_CAN_ID 0x1272F00

#define ERR __LINE__

// Signals
typedef enum {
	SIG_TACH = 0,
	SIG_SPEED,
	SIG_AN1,
	SIG_AN2,
	SIG_AN3,
	SIG_AN4,

	NSIG,
} Signal;

// Table Control filter.
// Used for writing/reading calibration tables.
static const CanId tblCtrlFilter = {
	.isExt = true,
	.eid = TAB_CTRL_CAN_ID};

// Signal Control filter.
// Used for writing/reading the CAN ID and encoding format of  each signal.
// See `doc/datafmt.pdf'.
static const CanId sigCtrlFilter = {
	.isExt = true,
	.eid = SIG_CTRL_CAN_ID};

// Receive buffer 0 mask.
// RXB0 receives Table Control and Signal Control frames.
static const CanId rxb0Mask = {
	.isExt = true,
	.eid = 0x1FFFFF00, // all but LSB
};

// Receive buffer 1 mask.
// RXB1 is used for receiving signals.
// The mask is permissive: all messages are accepted and filtered in software.
static const CanId rxb1Mask = {
	.isExt = true,
	.eid = 0u, // accept all messages
};

// Calibration tables in EEPROM
static const Table tbls[NSIG] = {
	[SIG_TACH] = {0ul*TAB_SIZE}, // tachometer
	[SIG_SPEED] = {1ul*TAB_SIZE}, // speedometer
	[SIG_AN1] = {2ul*TAB_SIZE}, // analog channels...
	[SIG_AN2] = {3ul*TAB_SIZE},
	[SIG_AN3] = {4ul*TAB_SIZE},
	[SIG_AN4] = {5ul*TAB_SIZE},
};

// EEPROM address of encoding format structure for each signal.
// Each of these addresses point to a SigFmt structure in the EEPROM.
static const EepromAddr sigFmtAddrs[NSIG] = {
	[SIG_TACH] = NSIG*TAB_SIZE + 0ul*SER_SIGFMT_SIZE, // tachometer
	[SIG_SPEED] = NSIG*TAB_SIZE + 1ul*SER_SIGFMT_SIZE, // speedometer
	[SIG_AN1] = NSIG*TAB_SIZE + 2ul*SER_SIGFMT_SIZE, // analog channels...
	[SIG_AN2] = NSIG*TAB_SIZE + 3ul*SER_SIGFMT_SIZE,
	[SIG_AN3] = NSIG*TAB_SIZE + 4ul*SER_SIGFMT_SIZE,
	[SIG_AN4] = NSIG*TAB_SIZE + 5ul*SER_SIGFMT_SIZE,
};

// Encoding format and CAN ID of each signal
static volatile SigFmt sigFmts[NSIG];

// Load signals' encoding formats and CAN IDs from EEPROM
static Status
loadSigFmts(void) {
	U8 oldGie, k;
	Status status;

	// Disable interrupts so the volatile address pointers can be passed safely
	oldGie = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	for (k = 0u; k < NSIG; k++) {
		status = serReadSigFmt(sigFmtAddrs[k], (SigFmt *)&sigFmts[k]);
		if (status != OK) {
			INTCONbits.GIE = oldGie; // restore previous interrupt setting
			return ERR;
		}
	}

	// Restore previous interrupt setting
	INTCONbits.GIE = oldGie;

	return OK;
}

// Transmit an error code (typically a line number) to the CAN bus.
static void
txErrFrame(Status err) {
	CanFrame frame;

	frame.id = (CanId){.isExt = true, .eid = ERR_CAN_ID};
	frame.rtr = false;
	frame.dlc = 2u;
	frame.data[0u] = (err >> 8u) & 0xFF;
	frame.data[1u] = (err >> 0u) & 0xFF;
	(void)canTx(&frame);
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
	canInit();
	dacInit();
	eepromInit();

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING);
	canSetMask0(&rxb0Mask); // RXB0 receives control messages
	canSetFilter0(&tblCtrlFilter); // Table Control Frames
	canSetFilter1(&sigCtrlFilter); // Signal Control Frames
	canSetMask1(&rxb1Mask); // RXB1 receives signal values
		// RXB1 messages are filtered in software
	canIE(true); // enable interrupts on MCP2515's INT pin
	canSetMode(CAN_MODE_NORMAL);

	// Load signals' encoding formats and CAN IDs from EEPROM
	status = loadSigFmts();
	if (status != OK) {
		txErrFrame(status);
		reset();
	}

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
// See `doc/datafmt.pdf'
static Status
handleTblCtrlFrame(const CanFrame *frame) {
	U8 tab, row;
	U32 key;
	U16 val;
	Status status;
	CanFrame response;

	// Extract table and row indices from ID
	tab = (frame->id.eid & 0xE0) >> 5u;
	row = frame->id.eid & 0x1F;
	if (tab >= NSIG || row >= TAB_ROWS) {
		return ERR;
	}

	if (frame->rtr) { // REMOTE
		status = tabRead(&tbls[tab], row, &key, &val);
		if (status != OK) {
			return ERR;
		}
		response.id = (CanId){
			.isExt = true,
			.eid = TAB_CTRL_CAN_ID | ((tab << 5u) & 0xE0) | (row & 0x1F)};
		response.rtr = false;
		response.dlc = 6u;
		serU32Be(response.data, key);
		serU16Be(response.data+4u, val);
		return canTx(&response);
	} else { // DATA
		if (frame->dlc != 6u) {
			return ERR;
		}
		key = deserU32Be(frame->data);
		val = deserU16Be(frame->data+4u);
		return tabWrite(&tbls[tab], row, key, val);
	}
}

// Transmit the response to a Signal Control REMOTE FRAME.
// The response is a Signal Control DATA FRAME containing the CAN ID
// and encoding format of the requested signal.
static Status
respondSigCtrl(Signal sig) {
	const volatile SigFmt *sigFmt;
	CanFrame response;

	if (sig >= NSIG) {
		return ERR;
	}
	sigFmt = &sigFmts[sig];

	response.id = (CanId){
		.isExt = true,
		.eid = SIG_CTRL_CAN_ID | (sig & 0xF),
	};
	response.rtr = false;
	response.dlc = 7u;

	// SigId
	if (sigFmt->id.isExt) { // extended
		serU32Be(response.data, sigFmt->id.eid & 0x1FFFFFFF);
		response.data[0u] |= 0x80; // EXIDE=1

	} else { // standard
		serU32Be(response.data, sigFmt->id.sid & 0x7FF);
	}

	// Encoding
	response.data[4u] = sigFmt->start;
	response.data[5u] = sigFmt->size;
	response.data[6u] = (U8)((sigFmt->order & 0x1) << 7u)
		| (U8)((sigFmt->isSigned) ? 0x40 : 0x00);

	return canTx(&response);
}

// Set the CAN ID and encoding format of a signal in response
// to a Signal Control DATA FRAME.
static Status
setSigFmt(const CanFrame *frame) {
	Signal sig;
	SigFmt sigFmt;
	Status status;

	// Extract signal number from ID
	sig = frame->id.eid & 0xF;
	if (sig >= NSIG) {
		return ERR;
	}

	// Prepare to unpack DATA FIELD
	if (frame->dlc != 7u) {
		return ERR;
	}

	// Unpack SigId
	if (frame->data[0u] & 0x80) { // EXIDE
		// Extended
		sigFmt.id.isExt = true;
		sigFmt.id.eid = deserU32Be(frame->data) & 0x1FFFFFFF;
	} else {
		// Standard
		sigFmt.id.isExt = false;
		sigFmt.id.sid = deserU16Be(frame->data+2u) & 0x7FF;
	}

	// Unpack Encoding
	sigFmt.start = frame->data[4u];
	sigFmt.size = frame->data[5u];
	sigFmt.order = (frame->data[6u] & 0x80) ? LITTLE_ENDIAN : BIG_ENDIAN;
	sigFmt.isSigned = frame->data[6u] & 0x40;

	// Save to EEPROM
	status = serWriteSigFmt(sigFmtAddrs[sig], &sigFmt);
	if (status != OK) {
		return ERR;
	}

	// Update copy in RAM
	sigFmts[sig] = sigFmt;

	return OK;
}

// Handle a Signal Control Frame.
// See `doc/datafmt.pdf'
static Status
handleSigCtrlFrame(const CanFrame *frame) {
	Signal sig;

	if (frame->rtr) { // REMOTE
		sig = frame->id.eid & 0xF;
		return respondSigCtrl(sig); // respond with the signal's CAN ID and encoding format
	} else { // DATA
		return setSigFmt(frame);
	}
}

// Generate the output signal being sent to one of the gauges.
// Raw is the raw signal value extracted from a CAN frame.
static Status
driveGauge(Signal sig, Number raw) {
	Status status;
	U16 val;

	// TODO: remove
	CanFrame frame;
	frame.id = (CanId){.isExt=false, .sid=0x123};
	frame.rtr = false;
	frame.dlc = 3u;
	frame.data[0u] = sig & 0xFF;
	frame.data[1u] = raw.type & 0xFF;
	frame.data[2u] = raw.u8;
	canTx(&frame);

	if (sig >= NSIG) {
		return ERR;
	}

	// Lookup gauge waveform value in EEPROM table
	status = tabLookup(&tbls[sig], raw, &val);
	if (status != OK) {
		return ERR;
	}

	switch (sig) {
	case SIG_TACH:
		// TODO
		break;
	case SIG_SPEED:
		// TODO
		break;
	case SIG_AN1:
		dacSet1a(val);
		break;
	case SIG_AN2:
		dacSet1b(val);
		break;
	case SIG_AN3:
		dacSet2a(val);
		break;
	case SIG_AN4:
		dacSet2b(val);
		break;
	default:
		return ERR; // invalid signal
	}

	// TODO
}

// Handle a frame potentially holding a signal value.
static Status
handleSigFrame(const CanFrame *frame) {
	Status status, result;
	Signal sig;
	Number raw;

	result = OK;

	// Search for signal with this ID
	// Exhaustive because message may contain multiple signals.
	for (sig = 0u; sig < NSIG; sig++) {
		if (canIdEq(&frame->id, (const CanId *)&sigFmts[sig].id)) {
			// Extract raw signal value from frame
			status = sigPluck((const SigFmt *)&sigFmts[sig], frame, &raw);
			if (status == OK) {
				status = driveGauge(sig, raw); // generate output signal
			}
			result |= status;
		}
	}

	return result;
}

void
__interrupt() isr(void) {
	U8 rxStatus;
	CanFrame frame;
	Status status;

	if (INTCONbits.INTF) { // CAN interrupt
		rxStatus = canRxStatus();
		switch (rxStatus & 0x7) { // check filter hit
		case 0u: // RXF0: calibration table control
			canReadRxb0(&frame);
			status = handleTblCtrlFrame(&frame);
			if (status != OK) {
				txErrFrame(status);
			}
			break;
		case 1u: // RXF1: signal ID control
			canReadRxb0(&frame);
			status = handleSigCtrlFrame(&frame);
			if (status != OK) {
				txErrFrame(status);
			}
			break;
		default: // message in RXB1
			canReadRxb1(&frame);
			(void)handleSigFrame(&frame);
		}
		INTCONbits.INTF = 0; // clear flag
	}
}
