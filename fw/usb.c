#include <xc.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#include <usb.h>
#include <usb_device.h>
#include <usb_device_cdc.h>

#include "types.h"
#include "eeprom.h"

#include "usb.h"

/***** Macros *****/

#define min(a, b) (((a) < (b)) ? (a) : (b))

/***** Constants *****/

// Safety counter
#define BAILOUT 100u

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

// Input buffer
typedef struct {
	U8 data[CDC_DATA_OUT_EP_SIZE];
	U8 len, head;
} RxQueue;

/***** Function Declarations *****/

// States
static State *idleState(void);
static State *echoState(void);
static State *writeEepromState(void);
static State *readEepromState(void);

/***** Global Variables *****/

static RxQueue rxBuf = {.len = 0u, .head = 0u};
static U8 txBuf[CDC_DATA_IN_EP_SIZE];

/***** Function Definitions *****/

// Rx a char from USB.
// Returns FAIL if no data.
static Status
getchar(char *c) {
	if (rxBuf.len <= 0u) {
		rxBuf.len = getsUSBUSART(rxBuf.data, sizeof(rxBuf.data));
		rxBuf.head = 0u;
	}

	if (rxBuf.len > 0u) {
		*c = rxBuf.data[rxBuf.head];
		rxBuf.head++;
		rxBuf.len--;
		return OK;
	} else {
		return FAIL;
	}
}

// Attempt to Rx the next char up to retries+1 times.
static Status
getcharBlock(char *c, U8 retries) {
	Status status;

	do {
		status = getchar(c);
		if (status == OK) {
			return OK;
		}

		USBDeviceTasks();
	} while (--retries + 1u);

	return FAIL;
}

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
static State *
idleState(void) {
	static State state;
	char opcode, junk;
	Status status;

	state.next = idleState;

	// Read opcode
	status = getchar(&opcode);
	if (status != OK) {
		// No data
		return &state;
	}

	// Skip space
	if (getchar(&junk) != OK) {
		// Incomplete command
		putsUSBUSART("nack incomplete command\n");
		return &state;
	}

	// State transition
	switch (opcode) {
	case 'e': state.next = echoState; break;
	case 'w': state.next = writeEepromState; break;
	case 'r': state.next = readEepromState; break;
	default: // invalid command
		rxBuf.len = 0u; // discard input
		putsUSBUSART("nack invalid command\n");
		break;
	}

	return &state;
}

// Handle "e" echo command.
static State *
echoState(void) {
	static State state;
	static U8 i = 0u;
	Status status;

	state.next = echoState;

	while (i < sizeof(txBuf)) {
		status = getchar((char *) &txBuf[i]);
		if (status == OK && txBuf[i++] == '\n') {
			// End of command
			state.next = idleState;
			break;
		} else if (status != OK) {
			// Wait to receive more data and continue on next call
			return &state;
		}
	}

	if (i > 0u) {
		putUSBUSART(txBuf, i);
		i = 0u;
	}

	return &state;
}

// Parse a 2-digit hex number and consume the space after it.
static Status
parseU8(U8 *n) {
	U8 i;
	char c;
	Status status;

	*n = 0u;
	for (i = 0u; i < 2u; i++) {
		*n <<= 4u;
		status = getcharBlock(&c, BAILOUT);
		if (status != OK) {
			return FAIL;
		}
		if (isdigit(c)) {
			*n += c - '0';
		} else if (c >= 'A' && c <= 'F') {
			*n += 10u + (c - 'A');
		} else if (c >= 'a' && c <= 'f') {
			*n += 10u + (c - 'a');
		} else {
			return FAIL;
		}
	}

	// Skip space
	status = getcharBlock(&c, BAILOUT);
	if (status != OK || !isspace(c)) {
		return FAIL;
	}

	return OK;
}

// Handle "w" write eeprom command.
static State *
writeEepromState(void) {
	static State state;
	U16 addr;
	U8 size;
	U8 i;
	char c;
	Status status;

	state.next = idleState;

	// Read <addrHi/Lo> and <size>
	if (parseU8(&addr.hi) != OK) {
		putsUSBUSART("nack bad <addrHi>\n");
		return &state;
	}
	if (parseU8(&addr.lo) != OK) {
		putsUSBUSART("nack bad <addrLo>\n");
		return &state;
	}
	if (parseU8(&size) != OK) {
		putsUSBUSART("nack bad <size>\n");
		return &state;
	}

	eepromWriteEnable();

	// Read <bytes> into buffer
	for (i = 0u; i < size; i++) {
		// Check for overflow
		if (i > 0u && (i%sizeof(txBuf)) == 0u) {
			eepromWrite(addr, txBuf, sizeof(txBuf));
			addU16(&addr, sizeof(txBuf));
		}

		// Receive byte
		status = getcharBlock((char *) &txBuf[i%sizeof(txBuf)], BAILOUT);
		if (status != OK) {
			putsUSBUSART("nack not enough bytes\n");
			eepromWriteDisable();
			return &state;
		}
	}

	// Flush buffer to eeprom
	eepromWrite(addr, txBuf, i%sizeof(txBuf));

	eepromWriteDisable();

	// Consume '\n'
	status = getcharBlock(&c, BAILOUT);
	if (status != OK || c != '\n') {
		putsUSBUSART("nack missing newline\n");
		return &state;
	}

	putsUSBUSART("ok\n");
	return &state;
}

// Handle "r" read eeprom command.
static State *
readEepromState(void) {
	static State state;
	static U16 addr;
	static U8 size = 0u;
	U8 chunkSize;

	state.next = idleState;

	if (size == 0u) {
		// First time called in this transaction

		// Read command, including '\n'
		if (parseU8(&addr.hi) != OK) {
			putsUSBUSART("nack bad <addrHi>\n");
			return &state;
		}
		if (parseU8(&addr.lo) != OK) {
			putsUSBUSART("nack bad <addrLo>\n");
			return &state;
		}
		if (parseU8(&size) != OK) {
			putsUSBUSART("nack bad <size>\n");
			return &state;
		}
	}

	// Read from eeprom into buffer
	chunkSize = min(size, sizeof(txBuf)-1u); // -1 to leave space for \n
	eepromRead(addr, txBuf, chunkSize);
	addU16(&addr, chunkSize);
	size -= chunkSize;

	// End of read?
	if (size == 0u) {
		// Done
		txBuf[chunkSize] = '\n';
		state.next = idleState;
	} else {
		// More data to read in next call
		state.next = readEepromState;
	}

	// Flush buffer to USB
	putUSBUSART(txBuf, chunkSize+1u); // +1 for \n

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
