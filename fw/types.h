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
typedef uint16_t U16;
typedef uint32_t U32;

typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;

// Number
typedef struct {
	enum {
		NUM_U8,
		NUM_U16,
		NUM_U32,
		NUM_I8,
		NUM_I16,
		NUM_I32,
	} type;
	U8 u8;
	U16 u16;
	U32 u32;
	I8 i8;
	I16 i16;
	I32 i32;
} Number;
