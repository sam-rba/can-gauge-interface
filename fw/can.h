/* Microchip MCP2515 CAN Controller
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * MCP2515 FOSC = 12MHz (48MHz/4 from PIC's CLKOUT)
 *
 * Usage:
 *
 * #include <stdint.h>
 * #include "types.h"
 * #include "can.h"
 */

// Bit timings (CNF1, CNF2, CNF3)
#define CAN_TIMINGS_10K 0xDE, 0xAD, 0x06 // BRP=30, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMINGS_20K 0xCF, 0xAD, 0x06 // BRP=15, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMINGS_50K 0xC6, 0xAD, 0x06 // BRP=6, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMINGS_100K 0xC3, 0xAD, 0x06 // BRP=3, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMINGS_125K 0xC4, 0x9A, 0x03 // BRP=4, PropSeg=3, PS1=4, PS2=4, SP=66.7%, SJW=4
#define CAN_TIMINGS_250K 0xC2, 0x9A, 0x03 // BRP=2, PropSeg=3, PS1=4, PS2=4, SP=66.7%, SJW=4
#define CAN_TIMINGS_500K 0xC1, 0x9A, 0x03 // BRP=1, PropSeg=3, PS1=4, PS2=4, SP=66.7, SJW=4
#define CAN_TIMINGS_800K "800kbps unsupported" // not possible with 12MHz clock
#define CAN_TIMINGS_1M "1Mbps unsupported" // not possible with 12MHz clock

void canInit(void);
