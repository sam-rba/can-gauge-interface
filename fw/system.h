#ifndef SYSTEM_H
#define SYSTEM_H

#include "fixed_address_memory.h"

// TRIS
enum {
	OUT = 0,
	IN = 1,
};

void sysInit(void);

#endif // SYSTEM_H
