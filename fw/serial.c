#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "eeprom.h"
#include "can.h"
#include "signal.h"

#include "serial.h"

Status
serWriteCanId(U16 addr, const CanId *id) {
	U8 buf[4u];

	// Copy ID to buffer
	if (id->isExt) { // extended
		buf[0u] = (id->eid>>0u) & 0xFF;
		buf[1u] = (id->eid>>8u) & 0xFF;
		buf[2u] = (id->eid>>16u) & 0xFF;
		buf[3u] = (id->eid>>24u) & 0x1F;
		buf[3u] |= 0x80; // set EID flag in bit 31
	} else { // standard
		buf[0u] = (id->sid>>0u) & 0xFF;
		buf[1u] = (id->sid>>8u) & 0x07;
		buf[2u] = 0u;
		buf[3u] = 0u; // clear EID flag in bit 32
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
		id->eid = ((U32)buf[0u] << 0u)
			| ((U32)buf[1u] << 8u)
			| ((U32)buf[2u] << 16u)
			| (((U32)buf[3u] & 0x1F) << 24u);
	} else {
		id->isExt = false; // standard
		id->sid = ((U16)buf[0u] << 0u)
			| (((U16)buf[1u] & 0x07) << 8u);
	}

	return OK;
}

Status
serWriteSigFmt(EepromAddr addr, const SigFmt *sig) {
	// TODO
}

Status
serReadSigFmt(EepromAddr addr, SigFmt *sig) {
	// TODO
}
