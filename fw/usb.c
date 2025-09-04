#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

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
	IDLE, // waiting for command
	CMD_START, // received opcode
	ECHO,
	WRITE_EEPROM,
	READ_EEPROM,
} State;

static uint8_t readBuf[CDC_DATA_OUT_EP_SIZE];
static uint8_t writeBuf[CDC_DATA_IN_EP_SIZE];
static State state = IDLE;

// Handle "e" echo command.
static void
echo(void) {
	static uint8_t readLen = 0u;
	static uint8_t ri, wi;

	if (!USBUSARTIsTxTrfReady()) {
		return;
	}

	if (readLen == 0u) {
		readLen = getsUSBUSART(readBuf, sizeof(readBuf));
		ri = 0u;
		wi = 0u;
	}

	while (readLen--) {
		if (wi > sizeof(writeBuf)) {
			// Overflow; flush
			putUSBUSART(writeBuf, sizeof(writeBuf));
			wi = 0u;
			return; // continue on next call
		}

		writeBuf[wi] = readBuf[ri];
		if (readBuf[ri] == '\n') {
			// End of command
			putUSBUSART(writeBuf, wi);
			readLen = 0u;
			state = IDLE;
			return;
		}

		ri++;
		wi++;
	}
}

// Handle "w" write eeprom command.
static void
writeEeprom(void) {
	// TODO
}

// Handle "r" read eeprom command.
static void readEeprom(void) {
	// TODO
}

static void
setCmdState(uint8_t opcode) {
	switch (opcode) {
	case 'e': // echo
		state = ECHO;
		break;
	case 'w':
		state = WRITE_EEPROM;
		break;
	case 'r':
		state = READ_EEPROM;
		break;
	}
}

void
usbTask(void) {
	static uint8_t opcode = 0u;

	USBDeviceTasks();

	if (USBGetDeviceState() < CONFIGURED_STATE) {
		return;
	}
	if (USBIsDeviceSuspended()) {
		return;
	}

	switch (state) {
	case IDLE:
		if (getsUSBUSART(&opcode, 1u) > 0) {
			state = CMD_START;
		}
		break;
	case CMD_START:
		// Skip space char
		if (getsUSBUSART(readBuf, 1u) > 0) {
			setCmdState(opcode);
		}
	case ECHO: echo();
		break;
	case WRITE_EEPROM: writeEeprom();
		break;
	case READ_EEPROM: readEeprom();
		break;
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
