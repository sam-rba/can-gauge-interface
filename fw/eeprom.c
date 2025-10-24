#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

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
	(void)spiTx(addr.hi);
	(void)spiTx(addr.lo);
}

static bool
isPageStart(U16 addr) {
	return (addr.lo % PAGE_SIZE) == 0;
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
		addr = addU16U8(addr, 1u);

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
