/* Microchip 25LC160C 2KiB EEPROM
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <xc.h>
 * #include <stdint.h>
 * #include "types.h"
 * #include "eeprom.h"
 */

// Pin mapping
#define EEPROM_CS_TRIS TRISCbits.TRISC5
#define EEPROM_CS LATCbits.LATC5

typedef U16 EepromAddr;

void eepromInit(void);
Status eepromWrite(EepromAddr addr, U8 data[], U8 size);
Status eepromRead(EepromAddr addr, U8 data[], U8 size);
