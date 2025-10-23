CC = xc8-cc
INCLUDES = -I ./
CFLAGS = -mcpu=pic16f1459 -std=c99 $(INCLUDES) -Wall
LDFLAGS = 

SYSTEST_DIR = tests/system

HEX = can_gauge.hex

SRC = $(shell find . -maxdepth 1 -name "*.c" ! -name "main.c")

OBJ = $(notdir $(SRC:.c=.p1))
HDR = $(wildcard *.h)

SYSTEST_SRC = $(wildcard $(SYSTEST_DIR)/*.c)
SYSTEST_OBJ = $(notdir $(SYSTEST_SRC:.c=.p1))
SYSTEST_HEX = $(SYSTEST_OBJ:.p1=.hex)

$(HEX): $(OBJ) main.p1
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.p1: %.c
	$(CC) $(CFLAGS) -c $<

$(SYSTEST_HEX): %.hex: %.p1 $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(SYSTEST_OBJ): %.p1: $(SYSTEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.hex  *.d *.p1 *.lst *.rlf *.o *.s *.sdb *.sym *.hxl *.elf *.cmf

systest: $(SYSTEST_HEX)

$(OBJ): $(HDR)

.PHONY: clean systest
