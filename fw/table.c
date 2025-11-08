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

// Linear interpolation.
static U16
interp(I32 x, I32 x1, U16 y1, I32 x2, U16 y2) {
	return (U16)(y1 + ((I32)y2-y1) * (x-x1) / (x2-x1));
}

Status
tabLookup(const Table *tab, I32 key, U16 *val) {
	U8 row;
	U32 utkey;
	I32 tkey1, tkey2;
	U16 tval1, tval2;
	Status status;

	// Search for key
	for (row = 0u; row < TAB_ROWS; row++) {
		status = tabRead(tab, row, &utkey, &tval1);
		if (status != OK) {
			return FAIL;
		}
		tkey1 = *(I32 *)&utkey;

		if (key == tkey1) { // found exact key
			*val = tval1; 
			return OK;
		} else if (key < tkey1) {
			if (row > 0u) { // not first row
				// Interpolate with previous row
				status = tabRead(tab, row-1u, &utkey, &tval2);
				if (status != OK) {
					return FAIL;
				}
				tkey2 = *(I32 *)&utkey;
				*val = interp(key, tkey1, tval1, tkey2, tval2);
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
