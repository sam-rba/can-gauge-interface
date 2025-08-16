#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#define VENDOR_ID 0x04d8
#define PRODUCT_ID 0x000a

#define ACM_CTRL_DTR 0x01
#define ACM_CTRL_RTS 0x02

#define TIMEOUT 1000

#define BUF_SIZE 64

// line encoding: 9600 8N1
// 9600 = 0x2580 LE
static unsigned char encoding[] = {0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08};

static struct libusb_device_handle *devh = NULL;

static int ep_in_addr = 0x82;
static int ep_out_addr = 0x02;

// Returns 0 on success
static int
setup(void) {
	int res;

	res = libusb_init(NULL);
	if (res < 0) {
		fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(res));
		return -1;
	}

	libusb_set_debug(NULL, 3);

	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (!devh) {
		fprintf(stderr, "Error finding USB device\n");
		return -1;
	}

	for (int ifNum = 0; ifNum < 2; ifNum++) {
		if (libusb_kernel_driver_active(devh, ifNum)) {
			libusb_detach_kernel_driver(devh, ifNum);
		}
		res = libusb_claim_interface(devh, ifNum);
		if (res < 0) {
			fprintf(stderr, "error claiming interface: %s\n", libusb_error_name(res));
			return -1;
		}
	}

	res = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS, 0, NULL, 0, 0);
	if (res < 0) {
		fprintf(stderr, "Error during control transfer: %s\n", libusb_error_name(res));
		return -1;
	}

	res = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0);
	if (res < 0) {
		fprintf(stderr, "Error during control transfer: %s\n", libusb_error_name(res));
		return -1;
	}

	return 0;
}

static void
teardown(void) {
	if (devh) {
		libusb_close(devh);
	}
	libusb_exit(NULL);
}

// Send bytes to USB
static void
txChars(unsigned char *data, int n) {
	int actualLen, res;

	actualLen = 0;
	while (actualLen < n) {
		res = libusb_bulk_transfer(devh, ep_out_addr, data, n, &actualLen, TIMEOUT);
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
rxChars(unsigned char *data, int size) {
	int res, actualLen;

	res = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actualLen, TIMEOUT);
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
sendCommand(void) {
	unsigned char buf[BUF_SIZE];
	int len;

	while ((len = scanline(buf, sizeof(buf))) != EOF) {
		txChars(buf, len);
		if (len > 0 && buf[len-1] == '\n') {
			break;
		}
	}
}

static void
printResponse(void) {
	unsigned char buf[BUF_SIZE];
	int len;

	while ((len = rxChars(buf, sizeof(buf)-1)) >= 0) {
		buf[len] = '\0';
		printf("%s", buf);
		if (buf[len] == '\n') {
			return;
		}
	}
}

int
main(int argc, char *argv[]) {
	int res;
	if ((res = setup()) != 0) {
		teardown();
		return res;
	}

	for (;;) {
		sendCommand();
		printResponse();
	}

	libusb_release_interface(devh, 0);

	teardown();
	return 0;
}
