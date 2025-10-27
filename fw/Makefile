CC = xc8-cc
INCLUDES = -I.
CFLAGS = -mcpu=pic16f1459 -std=c99 -Wall $(INCLUDES)
LDFLAGS = 

HEX = can_gauge.hex
SRC = $(shell find . -maxdepth 1 -name "*.c" ! -name "main.c")
OBJ = $(notdir $(SRC:.c=.p1))
HDR = $(wildcard *.h)

$(HEX): $(OBJ) main.p1
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.p1: %.c
	$(CC) $(CFLAGS) -c $<

$(OBJ): $(HDR)


SYSTEST_DIR = tests/system
SYSTEST_SRC = $(wildcard $(SYSTEST_DIR)/*.c)
SYSTEST_OBJ = $(notdir $(SYSTEST_SRC:.c=.p1))
SYSTEST_HEX = $(SYSTEST_OBJ:.p1=.hex)

$(SYSTEST_HEX): %.hex: %.p1 $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(SYSTEST_OBJ): %.p1: $(SYSTEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $<

systest: $(SYSTEST_HEX)


UTEST_DIR = tests/unit
UNITY_DIR = $(UTEST_DIR)/Unity/src
UTEST_CC = tcc
UTEST_INCLUDES = -I. -I $(UNITY_DIR)
UTEST_CFLAGS = -std=c99 -Wall $(UTEST_INCLUDES)
UTEST_LDFLAGS = 
UTEST_BIN = $(basename $(wildcard $(UTEST_DIR)/*.c))
UTEST_SRC = $(wildcard $(UTEST_DIR)/*.c) $(UNITY_DIR)/unity.c
UTEST_OBJ = $(UTEST_SRC:.c=.o)
UTEST_HDR = $(wildcard $(UTEST_DIR)/*.h) $(UNITY_DIR)/unity.h

utest: $(UTEST_BIN)
	for t in $^; do \
		$$t; \
	done

$(UTEST_BIN): $(UTEST_OBJ)
	$(UTEST_CC) $(UTEST_CFLAGS) $(UTEST_LDFLAGS) -o $@ $^

$(UTEST_OBJ): %.o: %.c
	$(UTEST_CC) -c -o $@ $(UTEST_CFLAGS) $<

$(UTEST_OBJ): $(UTEST_HDR)


clean:
	rm -f *.hex  *.d *.p1 *.lst *.rlf *.o *.s *.sdb *.sym *.hxl *.elf *.cmf \
		$(UTEST_OBJ) $(UTEST_BIN)

.PHONY: clean systest utest
