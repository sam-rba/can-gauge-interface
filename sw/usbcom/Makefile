CC = tcc
CFLAGS = -std=c99 -Wall
LDFLAGS = -lusb-1.0

usbcom: usbcom.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm usbcom
