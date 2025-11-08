#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "can.h"
#include "eeprom.h"
#include "signal.h"
#include "serial.h"

#include "table.h"

Status
tabWrite(const Table *tab, U8 k, U32 key, U16 val) {
	U16 addr;
	U8 row[sizeof(key) + sizeof(val)];

	if (k >= TAB_ROWS) {
		return FAIL;
	}

	addr = tab->offset + k*TAB_ROW_SIZE;
	serU32Be(row, key);
	serU16Be(row+sizeof(key), val);
	return eepromWrite(addr, row, sizeof(row));
}

Status
tabRead(const Table *tab, U8 k, U32 *key, U16 *val) {
	U16 addr;
	U8 row[sizeof(*key) + sizeof(*val)];
	Status status;

	if (k >= TAB_ROWS) {
		return FAIL;
	}

	addr = tab->offset + k*TAB_ROW_SIZE;
	status = eepromRead(addr, row, sizeof(row));
	*key = deserU32Be(row);
	*val = deserU16Be(row+sizeof(U32));
	return status;
}

Status
tabLookup(const Table *tab, Number key, U16 *val) {
	U8 row;
	Number tkey1, tkey2;
	U16 tval1, tval2;
	Status status;
	int ord;

	// Search for key
	for (row = 0u; row < (TAB_ROWS-1u); row++) {
		status = tabRead(tab, row, &tkey1.u32, &tval1);
		if (status != OK) {
			return FAIL;
		}

		status = u32ToNum(tkey1.u32, key.type, &tkey1);
		if (status != OK) {
			return FAIL;
		}

		status = cmpNum(key, tkey1, &ord);
		if (status != OK) {
			return FAIL;
		}
		if (ord == 0) { // found exact key
			*val = tval1; 
			return OK;
		} else if (ord < 0) { // key < tkey
			if (row > 0u) { // not first row
				// Interpolate with previous row
				status = tabRead(tab, row-1u, &tkey2.u32, &tval2);
				if (status != OK) {
					return FAIL;
				}
				status = u32ToNum(tkey2.u32, key.type, &tkey2);
				if (status != OK) {
					return FAIL;
				}
				*val = (tval1 + tval2) / 2u; // TODO: linear interpolation
			} else { // key < key of first row
				*val = tval1; // use first row value
			}
			return OK;
		} else { // key > tkey
			// continue until key <= tkey
		}
	}

	// Reached last row
	*val = tval1; // last value in table

	return OK;
}
