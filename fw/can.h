/* Microchip MCP2515 CAN Controller
 *
 * Device: PIC16F1459
 * Compiler: XC8 v3.00
 *
 * MCP2515 FOSC = 12MHz (48MHz/4 from PIC's CLKOUT)
 *
 * Usage:
 *
 * #include <xc.h>
 * #include <stdbool.h>
 * #include <stdint.h>
 * #include "types.h"
 * #include "can.h"
 */

// Pin mapping
#define CAN_CS_TRIS TRISAbits.TRISA5
#define CAN_CS LATAbits.LATA5

// Bit timings (CNF1, CNF2, CNF3)
#define CAN_TIMING_10K 0xDD, 0xAD, 0x06 // BRP=30, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMING_20K 0xCE, 0xAD, 0x06 // BRP=15, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMING_50K 0xC5, 0xAD, 0x06 // BRP=6, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMING_100K 0xC2, 0xAD, 0x06 // BRP=3, PropSeg=6, PS1=6, PS2=7, SP=65%, SJW=4
#define CAN_TIMING_125K 0xC3, 0x9A, 0x03 // BRP=4, PropSeg=3, PS1=4, PS2=4, SP=66.7%, SJW=4
#define CAN_TIMING_250K 0xC1, 0x9A, 0x03 // BRP=2, PropSeg=3, PS1=4, PS2=4, SP=66.7%, SJW=4
#define CAN_TIMING_500K 0xC0, 0x9A, 0x03 // BRP=1, PropSeg=3, PS1=4, PS2=4, SP=66.7%, SJW=4
#define CAN_TIMING_800K "800kbps unsupported" // not possible with 12MHz clock
#define CAN_TIMING_1M "1Mbps unsupported" // not possible with 12MHz clock

// MCP2515 modes of operation
typedef enum {
	CAN_MODE_NORMAL = 0x0,
	CAN_MODE_SLEEP = 0x1,
	CAN_MODE_LOOPBACK = 0x2,
	CAN_MODE_LISTEN_ONLY = 0x3,
	CAN_MODE_CONFIG = 0x4,
} CanMode;

// CAN identifier
typedef struct {
	bool isExt; // is extended
	union {
		U16 sid; // 11-bit standard ID -- ID[28:18]
		U32 eid; // 29-bit extended ID -- ID[28:0]
	};
} CanId;

// CAN frame
typedef struct {
	CanId id;
	bool rtr; // remote transmission request
	U8 dlc; // data length code
	U8 data[8];
} CanFrame;

// Initialize the MCP2515.
// Initial mode is Config.
void canInit(void);

// Set the operating mode of the MCP2515.
void canSetMode(CanMode mode);

// Set the bit timing parameters (bitrate, etc.).
// Pass one of the CAN_TIMING_x macros.
// The MCP2515 must be in Config mode.
void canSetBitTiming(U8 cnf1, U8 cnf2, U8 cnf3);

// Enable/disable RX-buffer-full interrupts on the MCP2515's INT pin.
void canIE(bool enable);

// Read RX status with RX STATUS instruction.
U8 canRxStatus(void);

// Read the frame in RXB0.
void canReadRxb0(CanFrame *frame);

// Read the frame in RXB1.
void canReadRxb1(CanFrame *frame);

// Transmit a frame to the CAN bus
Status canTx(const CanFrame *frame);

// Set the message acceptance mask of RXB0.
// The MCP2515 must be in Config mode.
void canSetMask0(const CanId *mask);

// Set the message acceptance mask of RXB1.
// The MCP2515 must be in Config mode.
void canSetMask1(const CanId *mask);

// Set message acceptance filter 0 (RXB0).
// The MCP2515 must be in Config mode.
void canSetFilter0(const CanId *filter);

// Set message acceptance filter 1 (RXB0).
// The MCP2515 must be in Config mode.
void canSetFilter1(const CanId *filter);

// Set message acceptance filter 2 (RXB1).
// The MCP2515 must be in Config mode.
void canSetFilter2(const CanId *filter);

// Set message acceptance filter 3 (RXB1).
// The MCP2515 must be in Config mode.
void canSetFilter3(const CanId *filter);

// Set message acceptance filter 4 (RXB1).
// The MCP2515 must be in Config mode.
void canSetFilter4(const CanId *filter);

// Set message acceptance filter 5 (RXB1).
// The MCP2515 must be in Config mode.
void canSetFilter5(const CanId *filter);
