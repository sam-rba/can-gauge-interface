#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "can.h"
#include "eeprom.h"

static const U16 addr = {0x00, 0x0A};

void
main(void) {
	U8 val;

	sysInit();
	spiInit();
	eepromInit();

	val = 0x8A;
	(void)eepromWrite(addr, &val, 1u); // write 0b1000_1010
	(void)eepromRead(addr, &val, 1u); // read 0b1000_1010
	val = 0x8E;
	(void)eepromWrite(addr, &val, 1u); // write 0b1000_1110
	(void)eepromRead(addr, &val, 1u); // read 0b1000_1110

	for (;;) {

	}
}
