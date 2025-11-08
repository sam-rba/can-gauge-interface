#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "eeprom.h"
#include "can.h"
#include "signal.h"

#include "serial.h"

void
serU16Be(U8 buf[2u], U16 n) {
	buf[0u] = (n >> 8u) & 0xFF;
	buf[1u] = (n >> 0u) & 0xFF;
}

U16
deserU16Be(const U8 buf[2u]) {
	return ((U16)buf[0u] << 8u)
		| ((U16)buf[1u] << 0u);
}

void
serU32Be(U8 buf[4u], U32 n) {
	buf[0u] = (n >> 24u) & 0xFF;
	buf[1u] = (n >> 16u) & 0xFF;
	buf[2u] = (n >> 8u) & 0xFF;
	buf[3u] = (n >> 0u) & 0xFF;
}

U32
deserU32Be(const U8 buf[4u]) {
	return ((U32)buf[0u] << 24u)
		| ((U32)buf[1u] << 16u)
		| ((U32)buf[2u] << 8u)
		| ((U32)buf[3u] << 0u);
}

Status
serWriteCanId(U16 addr, const CanId *id) {
	U8 buf[4u];

	// Copy ID to buffer
	if (id->isExt) { // extended
		serU32Be(buf, id->eid & 0x1FFFFFFF);
		buf[3u] |= 0x80; // set EID flag in bit 31
	} else { // standard
		serU32Be(buf, id->sid & 0x7FF);
	}

	return eepromWrite(addr, buf, sizeof(buf));
}

Status
serReadCanId(U16 addr, CanId *id) {
	U8 buf[sizeof(U32)];
	Status status;

	// Read
	status = eepromRead(addr, buf, sizeof(buf));
	if (status != OK) {
		return FAIL;
	}

	// Unpack
	if (buf[3u] & 0x80) { // bit 31 is standard/extended flag
		id->isExt = true; // extended
		id->eid = deserU32Be(buf) & 0x1FFFFFFF;
	} else {
		id->isExt = false; // standard
		id->sid = deserU32Be(buf) & 0x7FF;
	}

	return OK;
}

Status
serWriteSigFmt(EepromAddr addr, const SigFmt *sig) {
	Status status;
	U8 buf[3u];

	// ID
	status = serWriteCanId(addr, &sig->id);
	if (status != OK) {
		return status;
	}

	// Encoding
	buf[0u] = sig->start;
	buf[1u] = sig->size;
	buf[2u] = (U8)((sig->order & 0x01) << 7u)
		| ((sig->isSigned) ? 0x40 : 0x00);
	return eepromWrite(addr+SER_CANID_SIZE, buf, sizeof(buf));
}

Status
serReadSigFmt(EepromAddr addr, SigFmt *sig) {
	Status status;
	U8 buf[3u];

	// ID
	status = serReadCanId(addr, &sig->id);
	if (status != OK) {
		return status;
	}

	// Encoding
	status = eepromRead(addr+SER_CANID_SIZE, buf, sizeof(buf));
	if (status != OK) {
		return status;
	}
	sig->start = buf[0u];
	sig->size = buf[1u];
	sig->order = (buf[2u] & 0x80) ? LITTLE_ENDIAN : BIG_ENDIAN;
	sig->isSigned = buf[2u] & 0x40;

	return OK;
}
