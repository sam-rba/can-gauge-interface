#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "sys.h"
#include "types.h"
#include "spi.h"

#include "eeprom.h"

// 16-byte page ('C' variant)
#define PAGE_SIZE 16u

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

// Timing
enum {
	TIME_WRITE_CYCLE_MS = 5,
};

void
eepromInit(void) {
	EEPROM_CS_TRIS = OUT;
	EEPROM_CS = 1;

	eepromWriteDisable();
}

void
eepromWriteEnable(void) {
	EEPROM_CS = 0;
	(void)spiTx(CMD_WRITE_ENABLE);
	EEPROM_CS = 1;
}

void
eepromWriteDisable(void) {
	EEPROM_CS = 0;
	(void)spiTx(CMD_WRITE_DISABLE);
	EEPROM_CS = 1;
}

static bool
isWriteInProgress(void) {
	U8 status;

	EEPROM_CS = 0;
	(void)spiTx(CMD_READ_STATUS);
	status = spiTx(0x00);
	EEPROM_CS = 1;
	return status & STATUS_WIP;
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

void
eepromWrite(U16 addr, U8 *data, U8 size) {
	while (isWriteInProgress()) {} // wait for previous write to finish

	EEPROM_CS = 0; // pull chip-select low

	(void)spiTx(CMD_WRITE);
	spiTxAddr(addr);
	while (size--) {
		(void)spiTx(*data);
		data++;
		incU16(&addr);

		// Check if write crosses page boundary
		if (isPageStart(addr) && size) {
			// Write current page to memory
			EEPROM_CS = 1;
			while (isWriteInProgress()) {}
			EEPROM_CS = 0;

			// Start next page
			(void)spiTx(CMD_WRITE);
			spiTxAddr(addr);
		}
	}

	EEPROM_CS = 1; // release chip-select
}

void
eepromRead(U16 addr, U8 *data, U8 size) {
	EEPROM_CS = 0; // pull chip-select low

	(void)spiTx(CMD_READ);
	spiTxAddr(addr);
	while (size--) {
		*data = spiTx(0x00);
		data++;
	}

	EEPROM_CS = 1; // release chip-select
}
