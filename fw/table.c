#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "can.h"
#include "eeprom.h"

#include "table.h"

Status
tabWrite(const Table *tab, U8 k, U32 key, U16 val) {
	if (k >= TAB_ROWS) {
		return FAIL;
	}

	U16 addr = tab->offset + k*TAB_ROW_SIZE;
	U8 row[sizeof(key) + sizeof(val)] = {
		(key>>0u)&0xFF, (key>>8u)&0xFF, (key>>16u)&0xFF, (key>>24u)&0xFF, // LE
		(val>>0u)&0xFF, (val>>8u)&0xFF}; // LE
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
	*key = ((U32)row[0u]<<0u) | ((U32)row[1u]<<8u) | ((U32)row[2u]<<16u) | ((U32)row[3u]<<24u); // LE
	*val = ((U16)row[4u]<<0u) | ((U16)row[5u]<<8u); // LE
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
