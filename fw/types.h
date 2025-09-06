/* Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 */

typedef enum {
	OK,
	FAIL,
} Status;

typedef uint8_t U8;

typedef struct {
	U8 hi, lo;
} U16;

void addU16(U16 *a, U8 b);
