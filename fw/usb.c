#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <usb.h>
#include <usb_device.h>
#include <usb_device_cdc.h>

#include "usb.h"

/***** Constants *****/

// Line coding
// See struct USB_CDC_LINE_CODING
enum {
	DATA_RATE = 9600, // bps
	CHAR_FORMAT = NUM_STOP_BITS_1,
	PARITY_TYPE = PARITY_NONE,
	DATA_BITS = 8,
};

/***** Types *****/

// A state is a function that executes the state's action and returns the next state.
typedef struct State {
	struct State* (*next)(void);
} State;

/***** Function Declarations *****/

// States
static State *idleState(void);
static State *echoState(void);
static State *writeEepromState(void);
static State *readEepromState(void);

/***** Global Variables *****/

static uint8_t readBuf[CDC_DATA_OUT_EP_SIZE];
static uint8_t writeBuf[CDC_DATA_IN_EP_SIZE];
static uint8_t readLen = 0u;

/***** Function Definitions *****/

void
usbTask(void) {
	static State state = {idleState};

	USBDeviceTasks();

	if (USBGetDeviceState() < CONFIGURED_STATE) {
		return;
	}
	if (USBIsDeviceSuspended()) {
		return;
	}

	if (USBUSARTIsTxTrfReady()) {
		// Execute action and transition to next state
		state = *state.next();
	}

	CDCTxService();
}

// Read (the start of) a command from USB.
// Leaves <args> at the beginning of the read buffer.
static State *
idleState(void) {
	static State state;
	uint8_t opcode;

	readLen = getsUSBUSART(readBuf, sizeof(readBuf));
	if (readLen >= 2u) {
		opcode = readBuf[0u];

		// skip <opcode> <space>
		memmove(readBuf, readBuf+2u, readLen-2u);
		readLen -= 2u;

		switch (opcode) {
		case 'e': state.next = echoState; break;
		case 'w': state.next = writeEepromState; break;
		case 'r': state.next = readEepromState; break;
		default: state.next =  idleState; break; // invalid command
		}
	} else {
		// Invalid command. Must start with <opcode> <space>
		state.next = idleState;
	}

	return &state;
}

// Handle "e" echo command.
static State *
echoState(void) {
	static State state = {echoState};
	uint8_t i;

	if (readLen == 0u) {
		readLen = getsUSBUSART(readBuf, sizeof(readBuf));
	}

	for (i = 0u; i < readLen; i++) {
		writeBuf[i] = readBuf[i];
		if (readBuf[i] == '\n') {
			// End of command
			state.next = idleState;
			i++;
			break;
		}
	}

	if (readLen > 0u) {
		putUSBUSART(writeBuf, i);
	}
	readLen = 0u;

	return &state;
}

// Handle "w" write eeprom command.
static State *
writeEepromState(void) {
	static State state = {idleState};
	// TODO
	return &state;
}

// Handle "r" read eeprom command.
static State *
readEepromState(void) {
	static State state = {idleState};
	// TODO
	return &state;
}

static void
configure(void) {
	line_coding.dwDTERate = DATA_RATE;
	line_coding.bCharFormat = CHAR_FORMAT;
	line_coding.bParityType = PARITY_TYPE;
	line_coding.bDataBits = DATA_BITS;
}

bool
USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, uint16_t size) {
	switch ((int)event) {
	case EVENT_TRANSFER:
		break;

	case EVENT_SOF:
		break;

	case EVENT_SUSPEND:
		break;

	case EVENT_RESUME:
		break;

	case EVENT_CONFIGURED:
		CDCInitEP();
		configure();
		break;

	case EVENT_SET_DESCRIPTOR:
		break;

	case EVENT_EP0_REQUEST:
		USBCheckCDCRequest();
		break;

	case EVENT_BUS_ERROR:
		break;

	case EVENT_TRANSFER_TERMINATED:
		break;

	default:
		break;
	}

	return true;
}
