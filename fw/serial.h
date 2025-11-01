/** Serialization of structures stored in the EEPROM.
 *
 * Device PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdbool.h>
 * #include <stdint.h>
 * #include "types.h"
 * #include "eeprom.h"
 * #include "can.h"
 * #include "signal.h"
 * #include "serial.h"
 */

// Size of structures stored in ROM
enum {
	SER_CANID_SIZE = sizeof(U32),
	SER_SIGFMT_SIZE = 8,
};

// Write a CAN ID to the EEPROM.
// CAN IDs are stored in the lower 11 or 29 bits of a little-endian U32.
// Bit 31 indicates standard/extended: 0=>std, 1=>ext.
Status serWriteCanId(EepromAddr addr, const CanId *id);

// Read a CAN ID from the EEPROM.
// CAN IDs are stored in the lower 11 or 29 bits of a little-endian U32.
// Bit 31 indicates standard/extended: 0=>std, 1=>ext.
Status serReadCanId(EepromAddr addr, CanId *id);

// Write a SigFmt to the EEPROM.
// SigFmts use 8 bytes of space.
// The ID is stored little-endian in the first 4 bytes: 0--3.
// Start and Size occupy bytes 4 and 5.
// The Byte-order and Signedness flags are bits 0 and 1 of byte 6, respectively.
// Byte order: [6][0] = {0=>LE, 1=>BE}.
// Signedness: [6][1] = {0=>unsigned, 1=>signed}.
// Byte 7 is unused -- it's for alignment.
Status serWriteSigFmt(EepromAddr addr, const SigFmt *sig);

// Read a SigFmt from the EEPROM.
// See serWriteSigFmt for storage format.
Status serReadSigFmt(EepromAddr addr, SigFmt *sig);
