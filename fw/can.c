#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

#include "can.h"

// Oscillator startup timeout
#define STARTUP_TIME 128u

#define TX_RETRIES 10u

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

	// Transmit buffer 0
	REG_TXB0CTRL = 0x30, // control
	REG_TXB0SIDH = 0x31, // standard ID high
	REG_TXB0SIDL = 0x32, // standard ID low
	REG_TXB0EID8 = 0x33, // extended ID high
	REG_TXB0EID0 = 0x34, // extended ID low
	REG_TXB0DLC = 0x35, // data length code
	REG_TXB0DM = 0x36, // data byte <M> [36h+0, 36h+7]
} Reg;

// Masks
enum {
	// TXBnCTRL
	TXREQ = 0x08,
	TXERR = 0x10,

	// SIDL registers
	SRR = 0x10, // standard frame remote transmit request bit
	IDE = 0x08, // extended identifier flag bit

	// DLC registers
	RTR = 0x40,
};

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

// Write to a register.
static void
write(Reg addr, U8 data) {
	CAN_CS = 0;
	(void)spiTx(CMD_WRITE);
	(void)spiTx((U8)addr);
	(void)spiTx(data);
	CAN_CS = 1;
}

U8
canRxStatus(void) {
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

// Pack ID register values into a struct.
static void
packId(CanId *id, U8 sidh, U8 sidl, U8 eid8, U8 eid0) {
	if (sidl & IDE) { // extended ID
		id->isExt = true;
		id->eid = ((U32)eid0 << 0u) // id[7:0]
			| ((U32)eid8 << 8u) // id[15:8]
			| ((U32)((sidh << 5u) | (sidl >> 3u) | (sidl & 0x3)) << 16u) // id[23:21], id[20:18], id[17:16]
			| ((U32)(sidh >> 3u) << 24u); // id[28:24]
	} else { // standard ID
		id->isExt = false;
		id->sid = ((U16)((sidh << 3u) | (sidl >> 5u)) << 0u) // sid[7:0]
			| ((U16)(sidh >> 5u) << 8u); // sid[10:8]
	}
}

static void
readRxbn(U8 n, CanFrame *frame) {
	U8 sidh, sidl, eid8, eid0;

	CAN_CS = 0;

	// Start reading at RXBnSIDH
	(void)spiTx(CMD_READ_RX | (U8)((n & 1) << 2u));

	// Read ID
	sidh = spiTx(0u);
	sidl = spiTx(0u);
	eid8 = spiTx(0u);
	eid0 = spiTx(0u);
	packId(&frame->id, sidh, sidl, eid8, eid0);

	// Read DLC and RTR
	frame->dlc = spiTx(0u);
	frame->rtr = (sidl & IDE) ? (frame->dlc & RTR) : (sidl & SRR);
	frame->dlc &= 0xF;

	// Read data
	for (n = 0u; n < frame->dlc; n++) {
		frame->data[n] = spiTx(0u);
	}

	CAN_CS = 1;
}

void
canReadRxb0(CanFrame *frame) {
	readRxbn(0u, frame);
}

void
canReadRxb1(CanFrame *frame) {
	readRxbn(1u, frame);
}

// Write an ID to a set of {xSIDH, xSIDL, xEID8, xEID0} registers,
// e.g., RXMnSIDH etc.
static void
writeId(const CanId *id, Reg sidh, Reg sidl, Reg eid8, Reg eid0) {
	if (id->isExt) { // extended
		write(eid0, id->eid & 0xFF); // id[7:0]
		write(eid8, (id->eid >> 8u) & 0xFF); // id[15:8]
		write(sidl, ((id->eid >> 13u) & 0xE0) | IDE | ((id->eid >> 16u) & 0x03)); // id[20:18], IDE, id[17:16]
		write(sidh, (id->eid >> 21u) & 0xFF); // id[28:21]
	} else { // standard
		write(sidh, (id->sid >> 3u) & 0xFF);
		write(sidl, (id->sid << 5u) & 0xE0);
		write(eid8, 0u);
		write(eid0, 0u);
	}
}

Status
canTx(const CanFrame *frame) {
	U8 k, ctrl;

	// Set ID, DLC, and RTR
	writeId(&frame->id, REG_TXB0SIDH, REG_TXB0SIDL, REG_TXB0EID8, REG_TXB0EID0);
	write(REG_TXB0DLC, (frame->dlc & 0x0F) | ((frame->rtr) ? RTR : 0));

	// Copy data to registers
	for (k = 0u; k < frame->dlc; k++) {
		write(REG_TXB0DM+k, frame->data[k]);
	}

	// Send
	bitModify(REG_TXB0CTRL, TXREQ, TXREQ);
	k = 0u;
	do {
		ctrl = read(REG_TXB0CTRL);
		if (ctrl & TXERR) {
			// Error
			bitModify(REG_TXB0CTRL, TXREQ, 0); // cancel  transmission
			return FAIL;
		}
	} while ((ctrl & TXREQ) && (++k <= TX_RETRIES)); // transmission in progress

	return (k <= TX_RETRIES) ? OK : FAIL;
}

void
canSetMask0(const CanId *mask) {
	writeId(mask, REG_RXM0SIDH, REG_RXM0SIDL, REG_RXM0EID8, REG_RXM0EID0);
}

void
canSetMask1(const CanId *mask) {
	writeId(mask, REG_RXM1SIDH, REG_RXM1SIDL, REG_RXM1EID8, REG_RXM1EID0);
}

void
canSetFilter0(const CanId *filter) {
	writeId(filter, REG_RXF0SIDH, REG_RXF0SIDL, REG_RXF0EID8, REG_RXF0EID0);
}

void
canSetFilter1(const CanId *filter) {
	writeId(filter, REG_RXF1SIDH, REG_RXF1SIDL, REG_RXF1EID8, REG_RXF1EID0);
}

void
canSetFilter2(const CanId *filter) {
	writeId(filter, REG_RXF2SIDH, REG_RXF2SIDL, REG_RXF2EID8, REG_RXF2EID0);
}

void
canSetFilter3(const CanId *filter) {
	writeId(filter, REG_RXF3SIDH, REG_RXF3SIDL, REG_RXF3EID8, REG_RXF3EID0);
}

void
canSetFilter4(const CanId *filter) {
	writeId(filter, REG_RXF4SIDH, REG_RXF4SIDL, REG_RXF4EID8, REG_RXF4EID0);
}

void
canSetFilter5(const CanId *filter) {
	writeId(filter, REG_RXF5SIDH, REG_RXF5SIDL, REG_RXF5EID8, REG_RXF5EID0);
}

bool
canIdEq(const CanId *a, const CanId *b) {
	if (a->isExt != b->isExt) {
		return false;
	}
	if (a->isExt) {
		return a->eid == b->eid;
	} else {
		return a->sid == b->sid;
	}
}
