#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <usb.h>
#include <usb_device.h>
#include <usb_device_cdc.h>

#include "usb.h"

// Line coding
// See struct USB_CDC_LINE_CODING
enum {
	DATA_RATE = 9600, // bps
	CHAR_FORMAT = NUM_STOP_BITS_1,
	PARITY_TYPE = PARITY_NONE,
	DATA_BITS = 8,
};

// State
typedef enum {
	IDLE,
	ECHO,
	WRITE_EEPROM,
	READ_EEPROM,
} State;

static uint8_t readBuf[CDC_DATA_OUT_EP_SIZE];
static uint8_t writeBuf[CDC_DATA_IN_EP_SIZE];
static uint8_t readLen = 0u;

// Handle "e" echo command.
// Returns the next state.
static State
echo(void) {
	uint8_t i;
	State state = ECHO;

	if (readLen == 0u) {
		readLen = getsUSBUSART(readBuf, sizeof(readBuf));
	}

	for (i = 0u; i < readLen; i++) {
		writeBuf[i] = readBuf[i];
		if (readBuf[i] == '\n') {
			// End of command
			state = IDLE;
			i++;
			break;
		}
	}

	if (readLen > 0u) {
		putUSBUSART(writeBuf, i);
	}
	readLen = 0u;

	return state;
}

// Handle "w" write eeprom command.
// Returns the next state.
static State
writeEeprom(void) {
	// TODO
	return IDLE;
}

// Handle "r" read eeprom command.
// Returns the next state.
static State
readEeprom(void) {
	// TODO
	return IDLE;
}

static State
cmdState(uint8_t opcode) {
	switch (opcode) {
	case 'e': return ECHO;
	case 'w': return WRITE_EEPROM;
	case 'r': return READ_EEPROM;
	default: return IDLE; // invalid command
	}
}

// Read (the start of) a command from USB.
// Leaves <args> at the beginning of the read buffer.
// Returns the next state based on the opcode of the command.
static State
readCmd(void) {
	uint8_t opcode;

	readLen = getsUSBUSART(readBuf, sizeof(readBuf));
	if (readLen < 2u) {
		// Invalid command. Must start with <opcode> <space>
		return IDLE;
	}

	opcode = readBuf[0u];

	// skip <opcode> <space>
	memmove(readBuf, readBuf+2u, readLen-2u);
	readLen -= 2u;

	return cmdState(opcode);
}

void
usbTask(void) {
	static State state = IDLE;

	USBDeviceTasks();

	if (USBGetDeviceState() < CONFIGURED_STATE) {
		return;
	}
	if (USBIsDeviceSuspended()) {
		return;
	}

	if (USBUSARTIsTxTrfReady()) {
		switch (state) {
		case IDLE: state = readCmd(); break;
		case ECHO: state = echo(); break;
		case WRITE_EEPROM: state = writeEeprom(); break;
		case READ_EEPROM: state = readEeprom(); break;
		}
	}

	CDCTxService();
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
