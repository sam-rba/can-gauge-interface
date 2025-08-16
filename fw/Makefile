CC = xc8-cc
INCLUDES = -I./ -I/home/sam/prog/mla/v2018_11_26/framework/usb/inc
CFLAGS = -mcpu=pic16f1459 -std=c99 $(INCLUDES) -Wall
LDFLAGS = 

SYSTEST_DIR = tests/system

OUT = can_gauge
SRC = $(shell find . -name "*.c" ! -name "main.c" -maxdepth 1)
HDR = $(wildcard *.h)

OBJ = $(SRC:.c=.p1)
HEX = $(OUT).hex

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
	rm -f  $(OBJ) $(HEX) *.d *.p1 *.lst *.rlf *.o *.s *.sdb *.sym *.hxl *.elf *.cmf

systest: $(SYSTEST_HEX)

$(OBJ): $(HDR)

.PHONY: clean systest
