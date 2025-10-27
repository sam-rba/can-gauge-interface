#include <stdbool.h>
#include <stdint.h>

#include <unity.h>

#include <types.h>

void setUp(void) {}
void tearDown(void) {}

void
testAddU16(void) {
	setUp();
	// TODO
	TEST_ASSERT(false);
	tearDown();
}

void
testAddU16U8(void) {
	setUp();
	// TODO
	TEST_ASSERT(false);
	tearDown();
}

void
testLshiftU16(void) {
	setUp();
	// TODO
	TEST_ASSERT(false);
	tearDown();
}

void
testRshiftU16(void) {
	setUp();
	// TODO
	TEST_ASSERT(false);
	tearDown();
}

void
testCmpU16(void) {
	setUp();
	// TODO
	TEST_ASSERT(false);
	tearDown();
}

int
main(void) {
	UnityBegin(__FILE__);

	// Types
	RUN_TEST(testAddU16);
	RUN_TEST(testAddU16U8);
	RUN_TEST(testLshiftU16);
	RUN_TEST(testRshiftU16);
	RUN_TEST(testCmpU16);

	return UnityEnd();
}
