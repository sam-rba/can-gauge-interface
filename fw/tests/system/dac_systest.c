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

	dacSet1a((U16){0u, 252u}); // 1.23V

	for (;;) {

	}
}

void
__interrupt() isr(void) {
	static U8 ctr = 0u;
	static U16 level = {0, 0};

	if (TMR1IF) {
		ctr++;
		if (ctr == 23u) { // 1s period
			ctr = 0u;
			dacSet1a(level);
			addU16(&level, 50u);
		}
		TMR1IF = 0;
	}
}
