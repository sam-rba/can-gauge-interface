#ifndef SYSTEM_H
#define SYSTEM_H

#include "fixed_address_memory.h"

// Pins
#define TACH_PIN RC3
#define TACH_TRIS TRISC3

// TRIS
enum {
	OUT = 0,
	IN = 1,
};

void sysInit(void);

#endif // SYSTEM_H
