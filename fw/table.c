#include <xc.h>

#include <stdint.h>

#include "types.h"
#include "eeprom.h"

#include "table.h"

Status
tabWrite(const Table *tab, U8 k, U16 key, U16 val) {
	if (k >= TAB_ROWS) {
		return FAIL;
	}

	U16 addr = addU16U8(tab->offset, k*TAB_ROW_SIZE);
	U8 row[4u] = {key.lo, key.hi, val.lo, val.hi};
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

	addr = addU16U8(tab->offset, k*TAB_ROW_SIZE);
	status = eepromRead(addr, row, sizeof(row));
	*key = (U16){.lo=row[0u], .hi=row[1u]};
	*val = (U16){.lo=row[2u], .hi=row[3u]};
	return status;
}

Status
tabLookup(const Table *tab, U16 key, U16 *val) {
	U8 k;
	U16 tkey, tval1, tval2;
	Status status;
	I8 ord;

	// Search for key
	for (k = 0u; (k < TAB_ROWS-1u); k++) {
		status = tabRead(tab, k, &tkey, &tval1);
		if (status != OK) {
			return FAIL;
		}
		ord = cmpU16(key, tkey);
		if (ord == 0u) { // found exact key
			*val = tval1; 
			return OK;
		} else if (ord > 0u) { // interpolate
			status = tabRead(tab, k+1u, &tkey, &tval2);
			if (status != OK) {
				return FAIL;
			}
			// Mean: (tval1+tval2)/2
			*val = rshiftU16(addU16(tval1, tval2), 1u); 
			return OK;
		} else { // less
			// continue
		}
	}

	// Reached last row
	*val = tval1; // last value in table

	return OK;
}
