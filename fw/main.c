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

// Control frame CAN IDs
enum {
	TAB_CTRL_CAN_ID = 0x1272000, // Table Control Frame ID
	SIG_CTRL_CAN_ID = 0x1272100, // Signal Control Frame ID
};

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

	// TODO:
	// This is a stub to load hard-coded SigFmts until the serialization format is finalized.
	for (k = 0u; k < NSIG; k++) {
		sigFmts[k] = (SigFmt) {
			.id = {
				.isExt = true,
				.eid = 2365480958},
			.start = 24u,
			.size = 16u,
			.order = LITTLE_ENDIAN,
			.isSigned = false,
		}; // J1939 EngineSpeed
	}

	// Disable interrupts so the volatile address pointers can be passed safely
	oldGie = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	for (k = 0u; k < NSIG; k++) {
		status = serReadSigFmt(sigFmtAddrs[k], (SigFmt*)&sigFmts[k]);
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
	eepromInit();
	dacInit();
	canInit();

	// Load signals' encoding formats and CAN IDs from EEPROM
	status = loadSigFmts();
	if (status != OK) {
		reset();
	}

	// Setup MCP2515 CAN controller
	canSetBitTiming(CAN_TIMING);
	canSetMask0(&rxb0Mask); // RXB0 receives control messages
	canSetFilter0(&tblCtrlFilter); // Table Control Frames
	canSetFilter1(&sigCtrlFilter); // Signal Control Frames
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

// Transmit the response to a Signal Control REMOTE FRAME.
// The response is a Signal Control DATA FRAME containing the CAN ID
// and encoding format of the requested signal.
static Status
respondSigCtrl(Signal sig) {
	const volatile SigFmt *sigFmt;
	CanFrame response;

	if (sig >= NSIG) {
		return FAIL;
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
		response.data[0u] = 0x80 | ((sigFmt->id.eid >> 24u) & 0x1F); // EXIDE=1
		response.data[1u] = (sigFmt->id.eid >> 16u) & 0xFF;
		response.data[2u] = (sigFmt->id.eid >> 8u) & 0xFF;
		response.data[3u] = (sigFmt->id.eid >> 0u) & 0xFF;
	} else { // standard
		response.data[0u] = 0u; // EXIDE=0
		response.data[1u] = 0u;
		response.data[2u] = (sigFmt->id.sid >> 8u) & 0x07;
		response.data[3u] = (sigFmt->id.sid >> 0u) & 0xFF;
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
		return FAIL;
	}

	// Prepare to unpack DATA FIELD
	if (frame->dlc != 7u) {
		return FAIL;
	}

	// Unpack SigId
	if (frame->data[0u] & 0x80) { // EXIDE
		// Extended
		sigFmt.id.isExt = true;
		sigFmt.id.eid = (((U32)frame->data[0u] & 0x1F) << 24u)
			| ((U32)frame->data[1u] << 16u)
			| ((U32)frame->data[2u] << 8u)
			| ((U32)frame->data[3u] << 0u);
	} else {
		// Standard
		sigFmt.id.isExt = false;
		sigFmt.id.sid = (((U16)frame->data[2u] & 0x07) << 8u)
			| ((U16)frame->data[3u] << 0u);
	}

	// Unpack Encoding
	sigFmt.start = frame->data[4u];
	sigFmt.size = frame->data[5u];
	sigFmt.order = (frame->data[6u] & 0x80) ? BIG_ENDIAN : LITTLE_ENDIAN;
	sigFmt.isSigned = frame->data[6u] & 0x40;

	// Save to EEPROM
	status = serWriteSigFmt(sigFmtAddrs[sig], &sigFmt);
	if (status != OK) {
		return FAIL;
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

	if (sig >= NSIG) {
		return FAIL;
	}

	// Lookup gauge waveform value in EEPROM table
	status = tabLookup(&tbls[sig], raw, &val);
	if (status != OK) {
		return FAIL;
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
		return FAIL; // invalid signal
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

	if (INTCONbits.INTF) { // CAN interrupt
		rxStatus = canRxStatus();
		switch (rxStatus & 0x7) { // check filter hit
		case 0u: // RXF0: calibration table control
			canReadRxb0(&frame);
			(void)handleTblCtrlFrame(&frame);
			break;
		case 1u: // RXF1: signal ID control
			canReadRxb0(&frame);
			(void)handleSigCtrlFrame(&frame);
			break;
		default: // message in RXB1
			canReadRxb1(&frame);
			(void)handleSigFrame(&frame);
		}
		INTCONbits.INTF = 0; // clear flag
	}
}
