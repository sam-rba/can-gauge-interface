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

// Calibration tables in EEPROM
static const Table tachTbl = {0ul*TAB_SIZE}; // tachometer
static const Table speedTbl = {1ul*TAB_SIZE}; // speedometer
static const Table an1Tbl = {2ul*TAB_SIZE}; // analog channels...
static const Table an2Tbl = {3ul*TAB_SIZE};
static const Table an3Tbl = {4ul*TAB_SIZE};
static const Table an4Tbl = {5ul*TAB_SIZE};

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
	// TODO: "ID Control" will likely have to be renamed to "Signal Control" or "Encoding Control" or similar. It will carry a SigFmt structure instead of just a CAN ID. The datafmt doc will have to be updated too.
}

// Set the CAN ID associated with a signal in response to an ID Control DATA FRAME.
static Status
setSigId(const CanFrame *frame) {
	// TODO: this will likely have to be renamed to setSigFmt or similar. See above comment on updating datafmt doc.
}

// Handle an ID Control Frame.
// See `doc/datafmt.ps'
static Status
handleIdCtrlFrame(const CanFrame *frame) {
	Signal sig;

	// TODO: update datafmt doc to transceive entire signal encoding format instead of just the ID.
	if (frame->rtr) { // REMOTE
		sig = frame->id.eid & 0xF;
		return respondIdCtrl(sig); // respond with the signal's CAN ID
	} else { // DATA
		return setSigId(frame);
	}
}

// Generate the output signal being sent to one of the gauges.
static Status
driveGauge(Signal sig, Number raw) {
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
			// TODO: see above TODOs on updating datafmt doc
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
