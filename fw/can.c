#include <xc.h>

#include <stdint.h>

#include "types.h"

#include "can.h"

// Register addresses
enum {
	// Pin control and status
	REG_BFPCTRL = 0x0C, 

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
	REG_RXM0SIDH = 0x20, standard ID high
	REG_RXM0SIDL = 0x21, // standard ID low
	REG_RXM0EID8 = 0x22, // extended ID high
	REG_RXM0EID0 = 0x23, // extended ID low

	// Mask 1
	REG_RXM1SIDH = 0x24, // standard ID high
	REG_RXM1SIDL = 0x25, // standard ID low
	REG_RXM1EID8 = 0x26. // extended ID high
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
