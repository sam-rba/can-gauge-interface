#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

#include "can.h"

// Oscillator startup timeout
#define STARTUP_TIME 128u

// Register addresses
enum {
	// Config
	REG_BFPCTRL = 0x0C, // pin control and status
	REG_CANCTRL = 0x0F, // CAN control
	REG_CANSTAT = 0x0E, // CAN status

	// Bit timing
	REG_CNF1 = 0x2A,
	REG_CNF2 = 0x29,
	REG_CNF3 = 0x28,

	// Interrupts
	REG_CANINTE = 0x2B, // CAN interrupt enable
	REG_CANINTF = 0x2C, // CAN interrupt flag

	// Filter 0
	REG_RXF0SIDH = 0x00, // standard ID high
	REG_RXF0SIDL = 0x01, // standard ID low
	REG_RXF0EID8 = 0x02, // extended ID high
	REG_RXF0EID0 = 0x03, // extended ID low

	// Filter 1
	REG_RXF1SIDH = 0x04, // standard ID high
	REG_RXF1SIDL = 0x05, // standard ID low
	REG_RXF1EID8 = 0x06, // extended ID high
	REG_RXF1EID0 = 0x07, // extended ID low

	// Filter 2
	REG_RXF2SIDH = 0x08, // standard ID high
	REG_RXF2SIDL = 0x09, // standard ID low
	REG_RXF2EID8 = 0x0A, // extended ID high
	REG_RXF2EID0 = 0x0B, // extended ID low

	// Filter 3
	REG_RXF3SIDH = 0x10, // standard ID high
	REG_RXF3SIDL = 0x11, // standard ID low
	REG_RXF3EID8 = 0x12, // extended ID high
	REG_RXF3EID0 = 0x13, // extended ID low

	// Filter 4
	REG_RXF4SIDH = 0x14, // standard ID high
	REG_RXF4SIDL = 0x15, // standard ID low
	REG_RXF4EID8 = 0x16, // extended ID high
	REG_RXF4EID0 = 0x17, // extended ID low

	// Filter 5
	REG_RXF5SIDH = 0x18, // standard ID high
	REG_RXF5SIDL = 0x19, // standard ID low
	REG_RXF5EID8 = 0x1A, // extended ID high
	REG_RXF5EID0 = 0x1B, // extended ID low

	// Mask 0
	REG_RXM0SIDH = 0x20, // standard ID high
	REG_RXM0SIDL = 0x21, // standard ID low
	REG_RXM0EID8 = 0x22, // extended ID high
	REG_RXM0EID0 = 0x23, // extended ID low

	// Mask 1
	REG_RXM1SIDH = 0x24, // standard ID high
	REG_RXM1SIDL = 0x25, // standard ID low
	REG_RXM1EID8 = 0x26, // extended ID high
	REG_RXM1EID0 = 0x27, // extended ID low

	// Receive buffer 0
	REG_RXB0CTRL = 0x60, // control
	REG_RXB0SIDH = 0x61, // standard ID high
	REG_RXB0SIDL = 0x62, // standard ID low
	REG_RXB0EID8 = 0x63, // extended ID high
	REG_RXB0EID0 = 0x64, // extended ID low
	REG_RXB0DLC = 0x65, // data length code
	REG_RXB0DM = 0x66, // data byte <M> [66h+0, 66h+7]

	// Receive buffer 1
	REG_RXB1CTRL = 0x70, // control
	REG_RXB1SIDH = 0x71, // standard ID high
	REG_RXB1SIDL = 0x72, // standard ID low
	REG_RXB1EID8 = 0x73, // extended ID high
	REG_RXB1EID0 = 0x74, // extended ID low
	REG_RXB1DLC = 0x75, // data length code
	REG_RXB1DM = 0x76, // data byte <M> [76h+0, 76h+7]
};

// Modes of operation
typedef enum {
	MODE_NORMAL = 0x0,
	MODE_SLEEP = 0x1,
	MODE_LOOPBACK = 0x2,
	MODE_LISTEN_ONLY = 0x3,
	MODE_CONFIG = 0x4,
} Mode;

// Instructions
enum {
	CMD_RESET = 0xC0,
	CMD_READ = 0x03,
	CMD_READ_RX = 0x90,
	CMD_WRITE = 0x02,
	CMD_LOAD_TX = 0x40,
	CMD_RTS = 0x80,
	CMD_READ_STATUS = 0xA0,
	CMD_RX_STATUS = 0xB0,
	CMD_BIT_MODIFY = 0x05,
};

// Receive buffer identifiers
typedef enum {
	RXB0 = 0,
	RXB1 = 1,
} RxBuf;

// Send RESET instruction.
static void
reset(void) {
	CAN_CS = 0;
	(void)spiTx(CMD_RESET);
	CAN_CS = 1;
}

// Read n registers into data, starting at the specified address.
static void
read(U8 addr, U8 *data, U8 n) {
	CAN_CS = 0;
	(void)spiTx(CMD_READ);
	(void)spiTx(addr);
	while (n--) {
		*data = spiTx(0x00);
		data++;
	}
	CAN_CS = 1;
}

// Read the DATA field of one of the RX buffers.
static void
readRxData(RxBuf bufNum, U8 data[8u]) {
	U8 i;

	CAN_CS = 0;
	(void)spiTx(CMD_READ_RX | (U8)(bufNum << 2u) | 1u); // start at RXBnD0
	for (i = 0u; i < 8u; i++) {
		data[i] = spiTx(0x00);
	}
	CAN_CS = 1;
}

// Write to a register.
static void
write(U8 addr, U8 data) {
	CAN_CS = 0;
	(void)spiTx(CMD_WRITE);
	(void)spiTx(addr);
	(void)spiTx(data);
	CAN_CS = 1;
}

// Read RX status.
static U8
rxStatus(void) {
	U8 status;

	CAN_CS = 0;
	(void)spiTx(CMD_RX_STATUS);
	status = spiTx(0x00);
	CAN_CS = 1;
	return status;
}

// Write individual bits of a register with the BIT MODIFY instruction.
static void
bitModify(U8 addr, U8 mask, U8 data) {
	CAN_CS = 0;
	(void)spiTx(CMD_BIT_MODIFY);
	(void)spiTx(addr);
	(void)spiTx(mask);
	(void)spiTx(data);
	CAN_CS = 1;
}

void
canInit(void) {
	CAN_CS_TRIS = OUT;
	CAN_CS = 1;

	_delay(STARTUP_TIME); // wait for MCP2515 to power on
	reset(); // send RESET command
	_delay(STARTUP_TIME); // wait to power on again

	// TODO
}
