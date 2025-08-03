CC = xc8-cc
INCLUDES = -I./ -I/home/sam/prog/mla/v2018_11_26/framework/usb/inc
CFLAGS = -mcpu=pic16f1459 -std=c99 $(INCLUDES) -Wall
LDFLAGS = 

OUT = can_gauge
SRC = $(wildcard *.c)
HDR = $(wildcard *.h)

HEX = $(OUT).hex
OBJ = $(SRC:.c=.p1)

$(HEX): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.p1: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f  $(OBJ) $(HEX) *.d *.p1 *.lst *.rlf *.o *.s *.sdb *.sym *.hxl *.elf *.cmf

$(OBJ): $(HDR)

.PHONY: clean
