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
	U8 k;
	Number tkey;
	U16 tval1, tval2;
	Status status;
	int ord;

	// Search for key
	for (k = 0u; (k < TAB_ROWS-1u); k++) {
		status = tabRead(tab, k, &tkey.u32, &tval1);
		if (status != OK) {
			return FAIL;
		}

		status = u32ToNum(tkey.u32, key.type, &tkey);
		if (status != OK) {
			return FAIL;
		}

		status = cmpNum(key, tkey, &ord);
		if (status != OK) {
			return FAIL;
		}
		if (ord == 0) { // found exact key
			*val = tval1; 
			return OK;
		} else if (ord > 0) { // interpolate
			status = tabRead(tab, k+1u, &tkey.u32, &tval2);
			if (status != OK) {
				return FAIL;
			}
			*val = (tval1 + tval2) / 2u; 
			return OK;
		} else { // less
			// continue
		}
	}

	// Reached last row
	*val = tval1; // last value in table

	return OK;
}
