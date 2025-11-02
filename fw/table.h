/* Table datastructure for storing calibrations in the EEPROM.
 *
 * A table has a fixed number of rows that define a key/value mapping.
 * Keys and values are each U32.
 * T: U32->U32
 * Numbers are stored little-endian.
 *
 * Keys must be monotonically increasing.
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdbool.h>
 * #include <stdint.h>
 * #include "types.h"
 * #include "can.h"
 * #include "eeprom.h"
 * #include "table.h"
 */

enum {
	TAB_KEY_SIZE = sizeof(U32),
	TAB_VAL_SIZE = sizeof(U16),
	TAB_ROWS = 32,
	TAB_ROW_SIZE = TAB_KEY_SIZE + TAB_VAL_SIZE,
	TAB_SIZE = TAB_ROWS * TAB_ROW_SIZE,
};

typedef struct {
	EepromAddr offset; // starting address
} Table;

// Set the key and value of row k.
Status tabWrite(const Table *tab, U8 k, U32 key, U16 val);

// Read row k.
Status tabRead(const Table *tab, U8 k, U32 *key, U16 *val);

// Lookup the value associated with given key.
// If key falls between two rows, the value is interpolated
// from the two adjacent.
Status tabLookup(const Table *tab, Number key, U16 *val);
