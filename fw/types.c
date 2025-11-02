#include <stdint.h>

#include "types.h"

#define cmp(a, b) (((a) > (b)) ? 1 : (((a) < (b)) ? -1 : 0))

Status
u32ToNum(U32 x, NumType t, Number *n) {
	n->type = t;

	switch (t) {
	case NUM_U8:
		n->u8 = x & 0xFF;
		return OK;
	case NUM_U16:
		n->u16 = x & 0xFFFF;
		return OK;
	case NUM_U32:
		n->u32 = x;
		return OK;
	case NUM_I8:
		n->u8 = x & 0xFF;
		n->i8 = *(I8 *)&n->u8;
		return OK;
	case NUM_I16:
		n->u16 = x & 0xFFFF;
		n->i16 = *(I16 *)&n->u16;
		return OK;
	case NUM_I32:
		n->i32 = *(I32 *)&x;
		return OK;
	default:
		return FAIL;
	}
}

static Status
cmpU32Num(U32 a, Number b, int *ord) {
	switch (b.type) {
	case NUM_U8:
		*ord = cmp(a, b.u8);
		return OK;
	case NUM_U16:
		*ord = cmp(a, b.u16);
		return OK;
	case NUM_U32:
		*ord = cmp(a, b.u32);
		return OK;
	case NUM_I8:
		*ord = cmp(a, b.i8);
		return OK;
	case NUM_I16:
		*ord = cmp(a, b.i16);
		return OK;
	case NUM_I32:
		*ord = cmp(a, b.i32);
		return OK;
	default:
		return FAIL;
	}
}

Status
cmpNum(Number a, Number b, int *ord) {
	switch (a.type) {
	case NUM_U8:
		a.i32 = a.u8;
		break;
	case NUM_U16:
		a.i32 = a.u16;
		break;
	case NUM_U32:
		return cmpU32Num(a.u32, b, ord);
	case NUM_I8:
		a.i32 = a.i8;
		break;
	case NUM_I16:
		a.i32 = a.i16;
		break;
	case NUM_I32:
		break; // do nothing
	default:
		return FAIL;
	}

	switch (b.type) {
	case NUM_U8:
		*ord = cmp(a.i32, b.u8);
		return OK;
	case NUM_U16:
		*ord = cmp(a.i32, b.u16);
		return OK;
	case NUM_U32:
		*ord = cmp(a.i32, b.u32);
		return OK;
	case NUM_I8:
		*ord = cmp(a.i32, b.i8);
		return OK;
	case NUM_I16:
		*ord = cmp(a.i32, b.i16);
		return OK;
	case NUM_I32:
		*ord = cmp(a.i32, b.i32);
		return OK;
	default:
		return FAIL;
	}
}
