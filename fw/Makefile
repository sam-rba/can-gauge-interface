CC = xc8-cc
INCLUDES = -I ./ -I mla_usb/inc
CFLAGS = -mcpu=pic16f1459 -std=c99 $(INCLUDES) -Wall
LDFLAGS = 

SYSTEST_DIR = tests/system

HEX = can_gauge.hex

SRC = $(shell find . -name "*.c" ! -name "main.c" -maxdepth 1)

OBJ = $(notdir $(SRC:.c=.p1))
HDR = $(wildcard *.h)

USB_SRC = mla_usb/src/usb_device.c mla_usb/src/usb_device_cdc.c
USB_OBJ = $(notdir $(USB_SRC:.c=.p1))

SYSTEST_SRC = $(wildcard $(SYSTEST_DIR)/*.c)
SYSTEST_OBJ = $(notdir $(SYSTEST_SRC:.c=.p1))
SYSTEST_HEX = $(SYSTEST_OBJ:.p1=.hex)

$(HEX): $(OBJ) $(USB_OBJ) main.p1
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.p1: %.c
	$(CC) $(CFLAGS) -c $<

$(USB_OBJ): %.p1: mla_usb/src/%.c
	$(CC) $(CFLAGS) -c $<

$(SYSTEST_HEX): %.hex: %.p1 $(OBJ) $(USB_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(SYSTEST_OBJ): %.p1: $(SYSTEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f  $(OBJ) $(HEX) *.d *.p1 *.lst *.rlf *.o *.s *.sdb *.sym *.hxl *.elf *.cmf

systest: $(SYSTEST_HEX)

mla_usb:
	git submodule init mla_usb
	git submodule update mla_usb

$(OBJ): $(HDR)

.PHONY: clean systest
