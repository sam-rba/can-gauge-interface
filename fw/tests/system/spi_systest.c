#include <xc.h>

#include <stdint.h>

#include "system.h"
#include "types.h"
#include "spi.h"

void
main(void) {
	sysInit();
	spiInit();

	for (;;) {
		(void)spiTx(0x05); // 0b0000101
	}
}

void
__interrupt() isr(void) {

}
