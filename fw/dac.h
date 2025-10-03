/* Microchip MCP4912 10-bit DAC
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 * #include "dac.h"
 */

// Pin mapping
#define DAC1_CS_TRIS TRISBbits.TRISB7
#define DAC1_CS LATBbits.LATB7
#define DAC2_CS_TRIS TRISBbits.TRISB5
#define DAC2_CS LATBbits.LATB5

void dacInit(void);

// Set DAC1 VOUTA.
// Only the lower 10 bits are used.
void dacSet1a(U16 level);

// Set DAC1 VOUTB.
// Only the lower 10 bits are used.
void dacSet1b(U16 level);

// Set DAC2 VOUTA.
// Only the lower 10 bits are used.
void dacSet2a(U16 level);

// Set DAC2 VOUTB.
// Only the lower 10 bits are used.
void dacSet2b(U16 level);
