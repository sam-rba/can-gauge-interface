#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"
#include "dac.h"

void
main(void) {
	sysInit();
	spiInit();
	dacInit();

	dacSet1a(123); // 0.123V
	dacSet1b(345); // 0.345V
	dacSet2a(1230); // 1.230V
	dacSet2b(3450); // 3.450V

	for (;;) {

	}
}
