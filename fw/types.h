/* Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 */

typedef uint8_t U8;

typedef struct {
	U8 lo, hi;
} U16;

void incU16(U16 *x);
