#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "can.h"
#include "eeprom.h"

#include "table.h"

Status
tabWrite(const Table *tab, U8 k, U16 key, U16 val) {
	if (k >= TAB_ROWS) {
		return FAIL;
	}

	U16 addr = tab->offset + k*TAB_ROW_SIZE;
	U8 row[4u] = {
		(key>>0u)&0xFF, (key>>8u)&0xFF,
		(val>>0u)&0xFF, (val>>8u)&0xFF};
	return eepromWrite(addr, row, sizeof(row));
}

Status
tabRead(const Table *tab, U8 k, U16 *key, U16 *val) {
	U16 addr;
	U8 row[4u];
	Status status;

	if (k >= TAB_ROWS) {
		return FAIL;
	}

	addr = tab->offset + k*TAB_ROW_SIZE;
	status = eepromRead(addr, row, sizeof(row));
	*key = ((U16)row[0u]<<0u) | ((U16)row[1u]<<8u);
	*val = ((U16)row[2u]<<0u) | ((U16)row[3u]<<8u);
	return status;
}

Status
tabLookup(const Table *tab, U16 key, U16 *val) {
	U8 k;
	U16 tkey, tval1, tval2;
	Status status;

	// Search for key
	for (k = 0u; (k < TAB_ROWS-1u); k++) {
		status = tabRead(tab, k, &tkey, &tval1);
		if (status != OK) {
			return FAIL;
		}
		if (key == tkey) { // found exact key
			*val = tval1; 
			return OK;
		} else if (key > tkey) { // interpolate
			status = tabRead(tab, k+1u, &tkey, &tval2);
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
