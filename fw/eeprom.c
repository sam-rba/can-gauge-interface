#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "can.h"

#include "eeprom.h"

// 16-byte page ('C' variant)
#define PAGE_SIZE 16u

// Write cycle time = 5ms
// 5ms / (4*Tosc) = 60000
#define WRITE_DELAY 60000u

#define BAILOUT 10u

// Commands
enum {
	CMD_READ = 0x03,
	CMD_WRITE = 0x02,
	CMD_WRITE_DISABLE = 0x04,
	CMD_WRITE_ENABLE = 0x06,
	CMD_READ_STATUS = 0x05,
	CMD_WRITE_STATUS = 0x01,
};

// Status register masks
enum {
	STATUS_WIP = 0x1, // write in progress
	STATUS_WEL = 0x2, // write enable latch
	STATUS_BP0 = 0x4, // block protection 0
	STATUS_BP1 = 0x8, // block protection 1
};

static U8
readStatus(void) {
	U8 status;

	EEPROM_CS = 0;
	(void)spiTx(CMD_READ_STATUS);
	status = spiTx(0x00);
	EEPROM_CS = 1;
	return status;
}

static Status
writeEnable(void) {
	EEPROM_CS = 0;
	(void)spiTx(CMD_WRITE_ENABLE);
	EEPROM_CS = 1;

	return (readStatus() & STATUS_WEL) ? OK : FAIL;
}

static void
writeDisable(void) {
	EEPROM_CS = 0;
	(void)spiTx(CMD_WRITE_DISABLE);
	EEPROM_CS = 1;
}

void
eepromInit(void) {
	EEPROM_CS_TRIS = OUT;
	EEPROM_CS = 1;

	writeDisable();
}

// Wait for pending write to finish
static Status
waitForWrite(void) {
	U8 status, k;

	status = readStatus();
	for (k = 0u; (status & STATUS_WIP) && (k < BAILOUT); k++) {
		_delay(WRITE_DELAY);
		status = readStatus();
	}
	return (k < BAILOUT) ? OK : FAIL;
}

static void
spiTxAddr(U16 addr) {
	(void)spiTx((addr>>8u) & 0xFF); // MSB
	(void)spiTx((addr>>0u) & 0xFF); // LSB
}

static bool
isPageStart(U16 addr) {
	return (addr % PAGE_SIZE) == 0;
}

Status
eepromWrite(U16 addr, U8 *data, U8 size) {
	Status status;

	// Wait for pending write to finish
	status = waitForWrite();
	if (status != OK) {
		return FAIL; // timed out
	}

	// Set write-enable
	status = writeEnable();
	if (status != OK) {
		return FAIL;
	}

	// Write
	EEPROM_CS = 0;
	(void)spiTx(CMD_WRITE);
	spiTxAddr(addr);
	while (size--) {
		(void)spiTx(*data);
		data++;
		addr++;

		// Check if write crosses page boundary
		if (isPageStart(addr) && size) {
			// Write current page to memory
			EEPROM_CS = 1;
			status = waitForWrite();
			if (status != OK) {
				return FAIL; // timed out
			}
			status = writeEnable();
			if (status != OK) {
				return FAIL;
			}
			EEPROM_CS = 0;

			// Start next page
			(void)spiTx(CMD_WRITE);
			spiTxAddr(addr);
		}
	}
	EEPROM_CS = 1;

	return OK;
}

Status
eepromRead(U16 addr, U8 *data, U8 size) {
	Status status;

	// Wait for pending write to finish
	status = waitForWrite();
	if (status != OK) {
		return FAIL; // timed out
	}

	// Read
	EEPROM_CS = 0;
	(void)spiTx(CMD_READ);
	spiTxAddr(addr);
	while (size--) {
		*data = spiTx(0x00);
		data++;
	}
	EEPROM_CS = 1;

	return OK;
}

// Write a CAN ID to the EEPROM.
// CAN IDs are stored in the lower 11 or 29 bits of a little-endian U32.
// Bit 31 indicates standard/extended: 0=>std, 1=>ext.
Status
eepromWriteCanId(U16 addr, const CanId *id) {
	U8 buf[4u];

	// Copy ID to buffer
	if (id->isExt) { // extended
		buf[0u] = (id->eid>>0u) & 0xFF;
		buf[1u] = (id->eid>>8u) & 0xFF;
		buf[2u] = (id->eid>>16u) & 0xFF;
		buf[3u] = (id->eid>>24u) & 0x1F;
		buf[3u] |= 0x80; // set EID flag in bit 31
	} else { // standard
		buf[0u] = (id->sid>>0u) & 0xFF;
		buf[1u] = (id->sid>>8u) & 0x07;
		buf[2u] = 0u;
		buf[3u] = 0u; // clear EID flag in bit 32
	}

	return eepromWrite(addr, buf, sizeof(buf));
}

// Read a CAN ID from the EEPROM.
// CAN IDs are stored in the lower 11 or 29 bits of a little-endian U32.
// Bit 31 indicates standard/extended: 0=>std, 1=>ext.
Status
eepromReadCanId(U16 addr, CanId *id) {
	U8 buf[sizeof(U32)];
	Status status;

	// Read
	status = eepromRead(addr, buf, sizeof(buf));
	if (status != OK) {
		return FAIL;
	}

	// Unpack
	if (buf[3u] & 0x80) { // bit 31 is standard/extended flag
		id->isExt = true; // extended
		id->eid = ((U32)buf[0u] << 0u)
			| ((U32)buf[1u] << 8u)
			| ((U32)buf[2u] << 16u)
			| (((U32)buf[3u] & 0x1F) << 24u);
	} else {
		id->isExt = false; // standard
		id->sid = ((U16)buf[0u] << 0u)
			| (((U16)buf[1u] & 0x07) << 8u);
	}

	return OK;
}
