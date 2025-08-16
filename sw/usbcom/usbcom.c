#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#define TIMEOUT_MS 1000

#define BUF_SIZE 64

#define INTERFACE 0

enum {
	VENDOR_ID = 0x04d8,
	PRODUCT_ID = 0x000a,
};

enum {
	SET_LINE_CODING = 0x20,
	SET_CONTROL_LINE_STATE = 0x22,
};

enum {
	ACM_CTRL_DTR = 0x01,
	ACM_CTRL_RTS = 0x02,
};

enum {
	EP_OUT_ADDR = 0x02,
	EP_IN_ADDR = 0x82,
};

// line encoding: 9600 8N1
// 9600 = 0x2580 LE
static unsigned char encoding[] = {0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08};

// Returns NULL on error
static libusb_device_handle *
setup(void) {
	int res;
	libusb_device_handle *devh;

	res = libusb_init_context(NULL, NULL, 0);
	if (res < 0) {
		fprintf(stderr, "Error initializing libusb: %s\n", libusb_strerror(res));
		return NULL;
	}

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (!devh) {
		fprintf(stderr, "Error finding USB device\n");
		return NULL;
	}

	res = libusb_claim_interface(devh, INTERFACE);
	if (res < 0) {
		fprintf(stderr, "Failed to claim interface: %s\n", libusb_strerror(res));
		libusb_close(devh);
		return NULL;
	}

	res = libusb_control_transfer(devh, 0x21, SET_CONTROL_LINE_STATE, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, TIMEOUT_MS);
	if (res < 0) {
		fprintf(stderr, "Error during control transfer: %s\n", libusb_strerror(res));
		libusb_close(devh);
		return NULL;
	}

	res = libusb_control_transfer(devh, 0x21, SET_LINE_CODING, 0, 0, encoding, sizeof(encoding), TIMEOUT_MS);
	if (res < 0) {
		fprintf(stderr, "Error during control transfer: %s\n", libusb_strerror(res));
		libusb_close(devh);
		return NULL;
	}

	return devh;
}

static void
teardown(libusb_device_handle *devh) {
	libusb_release_interface(devh, INTERFACE);
	libusb_close(devh);
	libusb_exit(NULL);
}

// Send bytes to USB
static void
txChars(libusb_device_handle *devh, unsigned char *data, int n) {
	int actualLen, res;

	actualLen = 0;
	while (actualLen < n) {
		res = libusb_bulk_transfer(devh, EP_OUT_ADDR, data, n, &actualLen, TIMEOUT_MS);
		if (res == LIBUSB_ERROR_TIMEOUT) {
			printf("Timeout (%d)\n", actualLen);
		} else if (res < 0) {
			fprintf(stderr, "Error sending data\n");
		}
		assert(actualLen <= n);
		n -= actualLen;
		data += actualLen;
	}
}

// Read data from USB
// Returns number of bytes read, or -1 on error
static int
rxChars(libusb_device_handle *devh, unsigned char *data, int size) {
	int res, actualLen;

	res = libusb_bulk_transfer(devh, EP_IN_ADDR, data, size, &actualLen, TIMEOUT_MS);
	if (res == LIBUSB_ERROR_TIMEOUT) {
		fprintf(stderr, "Timeout\n");
		return -1;
	} else if (res < 0) {
		fprintf(stderr, "Error receiving data\n");
		return -1;
	}
	return actualLen;
}

// Get line from stdin
// Includes '\n'
// Returns number of chars prior to NUL, or EOF on end of file or error
static int
scanline(char *buf, size_t size) {
	int c, n;

	c = '\0';
	n = 0;
	while (n+1 < size) {
		c = getchar();
		if (c == EOF) {
			break;
		}
		buf[n++] = c;
		if (c == '\n') {
			break;
		}
	}

	if (size > 0) {
		buf[n] = '\0';
	}

	return ((c == EOF) && (n < 1)) ? EOF : n;
}

static void
sendCommand(libusb_device_handle *devh) {
	unsigned char buf[BUF_SIZE];
	int len;

	while ((len = scanline(buf, sizeof(buf))) != EOF) {
		txChars(devh, buf, len);
		if (len > 0 && buf[len-1] == '\n') {
			break;
		}
	}
}

static void
printResponse(libusb_device_handle *devh) {
	unsigned char buf[BUF_SIZE];
	int len;

	while ((len = rxChars(devh, buf, sizeof(buf)-1)) >= 0) {
		buf[len] = '\0';
		printf("%s", buf);
		if (buf[len] == '\n') {
			return;
		}
	}
}

int
main(int argc, char *argv[]) {
	libusb_device_handle *devh;

	devh = setup();
	if (!devh) {
		return 1;
	}

	for (;;) {
		sendCommand(devh);
		printResponse(devh);
	}

	teardown(devh);
	return 0;
}
