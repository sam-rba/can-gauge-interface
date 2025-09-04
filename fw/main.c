#include <xc.h>

#include <stdint.h>

#include <usb_device.h>

#include "types.h"
#include "init.h"
#include "spi.h"
#include "eeprom.h"
#include "usb.h"

void
main(void) {
	clockInit();
	pinsInit();
	spiInit();
	eepromInit();
	USBDeviceInit();
	USBDeviceAttach();

	for (;;) {
		usbTask();
	}
}

void
__interrupt() isr(void) {

}
