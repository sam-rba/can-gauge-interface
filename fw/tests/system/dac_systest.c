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
