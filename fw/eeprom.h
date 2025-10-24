/* Microchip 25LC160C 2KiB EEPROM
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 * #include "eeprom.h"
 */

// Pin mapping
#define EEPROM_CS_TRIS TRISCbits.TRISC5
#define EEPROM_CS LATCbits.LATC5

void eepromInit(void);
Status eepromWrite(U16 addr, U8 data[], U8 size);
Status eepromRead(U16 addr, U8 data[], U8 size);
