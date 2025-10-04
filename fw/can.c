#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

#include "can.h"

// Oscillator startup timeout
#define STARTUP_TIME 128u

// Register addresses
typedef enum {
	// Config and status
	REG_CANCTRL = 0x0F, // CAN control
	REG_CANSTAT = 0x0E, // CAN status
	REG_BFPCTRL = 0x0C, // pin control and status
	REG_RXB0CTRL = 0x60, // receive buffer 0 control
	REG_RXB1CTRL = 0x70, // receive buffer 1 control
	REG_CANINTE = 0x2B, // CAN interrupt enable
	REG_CANINTF = 0x2C, // CAN interrupt flags

	// Bit timing
	REG_CNF1 = 0x2A,
	REG_CNF2 = 0x29,
	REG_CNF3 = 0x28,

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
	REG_RXB0SIDH = 0x61, // standard ID high
	REG_RXB0SIDL = 0x62, // standard ID low
	REG_RXB0EID8 = 0x63, // extended ID high
	REG_RXB0EID0 = 0x64, // extended ID low
	REG_RXB0DLC = 0x65, // data length code
	REG_RXB0DM = 0x66, // data byte <M> [66h+0, 66h+7]

	// Receive buffer 1
	REG_RXB1SIDH = 0x71, // standard ID high
	REG_RXB1SIDL = 0x72, // standard ID low
	REG_RXB1EID8 = 0x73, // extended ID high
	REG_RXB1EID0 = 0x74, // extended ID low
	REG_RXB1DLC = 0x75, // data length code
	REG_RXB1DM = 0x76, // data byte <M> [76h+0, 76h+7]
} Reg;

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

// Read a register.
static U8
read(Reg addr) {
	U8 data;

	CAN_CS = 0;
	(void)spiTx(CMD_READ);
	(void)spiTx((U8)addr);
	data = spiTx(0x00);
	CAN_CS = 1;
	return data;
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
write(Reg addr, U8 data) {
	CAN_CS = 0;
	(void)spiTx(CMD_WRITE);
	(void)spiTx((U8)addr);
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
bitModify(Reg addr, U8 mask, U8 data) {
	CAN_CS = 0;
	(void)spiTx(CMD_BIT_MODIFY);
	(void)spiTx((U8)addr);
	(void)spiTx(mask);
	(void)spiTx(data);
	CAN_CS = 1;
}

void
canInit(void) {
	// Configure chip-select pin
	CAN_CS_TRIS = OUT;
	CAN_CS = 1;

	// Send RESET command
	_delay(STARTUP_TIME); // wait for MCP2515 to power on
	reset(); // send RESET command
	_delay(STARTUP_TIME); // wait to power on again

	// Configure registers
	canSetMode(CAN_MODE_CONFIG);
	bitModify(REG_CANCTRL, 0x1F, 0x00); // disable one-shot mode and CLKOUT pin
	write(REG_BFPCTRL, 0x00); // disable RXnBF interrupt pins
	write(REG_RXB0CTRL, 0x00); // use filters, disable rollover
	write(REG_RXB1CTRL, 0x00); // use filters
}

void
canSetMode(CanMode mode) {
	CanMode currentMode;

	do {
		// Set REQOP of CANCTRL to request mode
		bitModify(REG_CANCTRL, 0xE0, (U8) (mode << 5u));
		// Read OPMOD of CANSTAT
		currentMode = read(REG_CANSTAT) >> 5u;
	} while (currentMode != mode);
}

void
canSetBitTiming(U8 cnf1, U8 cnf2, U8 cnf3) {
	write(REG_CNF1, cnf1);
	write(REG_CNF2, cnf2);
	write(REG_CNF3, cnf3);
}

void
canIE(bool enable) {
	if (enable) {
		write(REG_CANINTF, 0x00); // clear interrupt flags
		write(REG_CANINTE, 0x03); // enable RX-buffer-full interrupts
	} else {
		write(REG_CANINTE, 0x00); // disable interrupts
	}
}

static void
writeIdToRegs(const CanId *id, Reg sidh, Reg sidl, Reg eid8, Reg eid0) {
	switch (id->type) {
	case CAN_ID_STD: // standard ID
		write(sidh, (U8)(id->sid.hi << 5u) | (U8)(id->sid.lo >> 3u));
		write(sidl, (U8) (id->sid.lo << 5u));
		write(eid8, 0x00);
		write(eid0, 0x00);
		break;
	case CAN_ID_EXT: // extended ID
		write(sidh, (U8)(id->eid[1] << 5u) | (U8)(id->eid[0] >> 3u)); // sid[10:3]
		write(sidl, (U8)(id->eid[0] << 5u) | (U8)0x08 | (U8)((id->eid[3] >> 3u) & 0x03)); // sid[2:0], exide, eid[28:27]
		write(eid8, (U8)(id->eid[3] << 5u) | (U8)(id->eid[2] >> 3u)); // eid[26:19]
		write(eid0, (U8)(id->eid[2] << 5u) | (U8)(id->eid[1] >> 3u)); // eid[18:11]
		break;
	}
}

void
canSetMask0(const CanId *mask) {
	writeIdToRegs(mask, REG_RXM0SIDH, REG_RXM0SIDL, REG_RXM0EID8, REG_RXM0EID0);
}

void
canSetMask1(const CanId *mask) {
	writeIdToRegs(mask, REG_RXM1SIDH, REG_RXM1SIDL, REG_RXM1EID8, REG_RXM1EID0);
}

void
canSetFilter0(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF0SIDH, REG_RXF0SIDL, REG_RXF0EID8, REG_RXF0EID0);
}

void
canSetFilter1(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF1SIDH, REG_RXF1SIDL, REG_RXF1EID8, REG_RXF1EID0);
}

void
canSetFilter2(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF2SIDH, REG_RXF2SIDL, REG_RXF2EID8, REG_RXF2EID0);
}

void
canSetFilter3(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF3SIDH, REG_RXF3SIDL, REG_RXF3EID8, REG_RXF3EID0);
}

void
canSetFilter4(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF4SIDH, REG_RXF4SIDL, REG_RXF4EID8, REG_RXF4EID0);
}

void
canSetFilter5(const CanId *filter) {
	writeIdToRegs(filter, REG_RXF5SIDH, REG_RXF5SIDL, REG_RXF5EID8, REG_RXF5EID0);
}
