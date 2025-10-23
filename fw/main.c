#include <xc.h>

#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "eeprom.h"
#include "can.h"

void
main(void) {
	sysInit();
	spiInit();
	eepromInit();
	canInit();

	for (;;) {

	}
}

void
__interrupt() isr(void) {

}
