#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "table.h"
#include "spi.h"
#include "eeprom.h"
#include "dac.h"
#include "can.h"

// Parameters:
// Each parameter has an associated CAN ID and calibration table in the EEPROM.
enum {
	TACH = 0,
	SPEED = 1,
	AN1 = 2,
	AN2 = 3,
	AN3 = 4,
	AN4 = 5,
} Parameter;

// CAN ID for writing/reading calibration tables from a host PC.
// LSB of ID, 'X', indicates table and row number: X[7:5]{table #}, X[4:0]{row #}.
static const CanId tblCanId = {
	.type = CAN_ID_EXT,
	.eid = {0x00, 0x2F, 0x27, 0x01}, // 1272F0Xh
};

// CAN ID for writing/reading the CAN ID associated with each parameter
// from a host PC.
// LSB of ID, 'X', indicates a particular Parameter enum.
static const CanId paramCanId = {
	.type = CAN_ID_EXT,
	.eid = {0x10, 0x2F, 0x27, 0x01}, // 1272F1Xh
};

// Receive buffer 0 mask.
// RXB0 is used for control messages for reading/writing configuration tables and CAN IDs for each parameter, which are stored in the EEPROM.
// Mask all but the LSB of the Table and Parameter IDs.
static const CanId rxb0Mask = {
	.type = CAN_ID_EXT,
	.eid = {0x10, 0x2F, 0x27, 0x01}, // 1272F10h
};

// Calibration tables in EEPROM:
static Table tachTbl;
static Table speedTbl;
static Table an1Tbl;
static Table an2Tbl;
static Table an3Tbl;

// Filter ID addresses in EEPROM:
// Each one of these addresses holds a CanId struct.
static U16 id1Addr;
static U16 id2Addr;
static U16 id3Addr;
static U16 id4Addr;
static U16 id5Addr;

void
main(void) {
	U16 offset;

	// Initialize calibration table EEPROM addresses
	offset = (U16){0u, 0u};
	tachTbl = (Table){offset};
	offset = addU16U8(offset, TAB_SIZE);
	speedTbl = (Table){offset};
	offset = addU16U8(offset, TAB_SIZE);
	an1Tbl = (Table){offset};
	offset = addU16U8(offset, TAB_SIZE);
	an2Tbl = (Table){offset};
	offset = addU16U8(offset, TAB_SIZE);
	an3Tbl = (Table){offset};
	offset = addU16U8(offset, TAB_SIZE);

	// Initialize filter ID EEPROM addresses
	id1Addr = offset;
	offset = addU16U8(offset, sizeof(CanId));
	id2Addr = offset;
	offset = addU16U8(offset, sizeof(CanId));
	id3Addr = offset;
	offset = addU16U8(offset, sizeof(CanId));
	id4Addr = offset;
	offset = addU16U8(offset, sizeof(CanId));
	id5Addr = offset;
	offset = addU16U8(offset, sizeof(CanId));

	sysInit();
	spiInit();
	eepromInit();
	dacInit();
	canInit();

	for (;;) {

	}
}

void
__interrupt() isr(void) {

}
