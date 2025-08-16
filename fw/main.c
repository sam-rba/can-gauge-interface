#include <xc.h>

#include <stdint.h>

#include "types.h"
#include "init.h"
#include "spi.h"
#include "eeprom.h"
#include "config.h"

void
main(void) {
	clockInit();
	pinsInit();
	spiInit();
	eepromInit();

	for (;;) {

	}
}

void
__interrupt() isr(void) {

}
