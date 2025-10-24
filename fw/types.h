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
typedef int8_t I8;

typedef struct {
	U8 hi, lo;
} U16;

// Little-endian 32-bit unsigned integer.
typedef U8 U32[4];

// a + b
U16 addU16(U16 a, U16 b);

// a + b
U16 addU16U8(U16 a, U8 b);

// a << b
U16 lshiftU16(U16 a, U8 b);

// a >> b
U16 rshiftU16(U16 a, U8 b);

// -1 if a < b
// 0 if a == b
// +1 if a > b
I8 cmpU16(U16 a, U16 b);
