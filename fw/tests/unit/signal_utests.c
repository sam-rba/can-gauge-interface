#include <stdbool.h>
#include <stdint.h>

#include <unity.h>

#include <types.h>
#include <can.h>
#include <signal.h>
#include <signal_utestable.h>

void setUp(void) {}
void tearDown(void) {}

static void
testPluckLE(void) {
	setUp();

	// TODO
	TEST_ASSERT(0);

	tearDown();
}

static void
testPluckBE(void) {
	setUp();

	// 1111 1111  (1110 11)10  (1111 0010)  1111 1(101)
	// 7       0  15        8  23      16  31       24
	CanFrame frame = {.data = {0xFF, 0xEE, 0xF2, 0xFD}};
	SigFmt sig = {
		.start = 10u,
		.size = 17u,
	};
	U32 want = 0x1DF95;
	U32 got;
	pluckBE(&sig, &frame, &got);
	TEST_ASSERT_EQUAL_UINT32(want, got);

	tearDown();
}

int
main(void) {
	UnityBegin(__FILE__);

	RUN_TEST(testPluckLE);
	RUN_TEST(testPluckBE);

	return UnityEnd();
}
