/** DBC Signals.
 * See "DBC File Format Documentation" v.01/2007
 *
 * Device PIC16F1459
 * Compiler: XC8 v3.00
 *
 * Usage:
 *
 * #include <stdbool.h>
 * #include <stdint.h>
 * #include "types.h"
 * #include "can.h"
 * #include "signal.h"
 */

// Byte order
typedef enum {
	BIG_ENDIAN = 0,
	LITTLE_ENDIAN,
} ByteOrder;

// A Signal Format defines how a DBC signal is encoded in a CAN frame.
typedef struct {
	CanId id; // ID of message containing the signal
	U8 start; // start bit -- position of signal within DATA FIELD
	U8 size; // size of the signal in bits
	ByteOrder order; // big-endian/little-endian
	bool isSigned;
} SigFmt;

// Extract the raw signal value out of a CAN frame's DATA FIELD.
// Assumes the frame's ID matches that of the signal.
Status sigPluck(const SigFmt *sig, const CanFrame *frame, I32 *raw);
