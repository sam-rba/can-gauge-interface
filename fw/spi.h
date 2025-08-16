/* Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 * #include "spi.h"
 */

void spiInit(void);
U8 spiTx(U8 c);
