// ============================================================================
//  Name        : musg2tool.c
//  Author      : hypothermic
//  Version     : 1.0.0
//  Copyright   : hypothermic, 2018
//  Description : Control the features of your Gembird MUSG-02 mouse.
// ============================================================================

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define VID 0x1ea7
#define PID 0x002d
#define IFACE 1

libusb_context *context = NULL;
libusb_device **list = NULL;
int rc = 0;
ssize_t count = 0;

libusb_device *searchForDevice() {
	count = libusb_get_device_list(context, &list);
	assert(count > 0);
	for (size_t idx = 0; idx < count; ++idx) {
		libusb_device *device = list[idx];
		struct libusb_device_descriptor desc = { 0 };

		rc = libusb_get_device_descriptor(device, &desc);
		assert(rc == 0);

		if (desc.idVendor == VID && desc.idProduct == PID) {
			return device;
		}
	}
	return NULL;
}

int submitControlRq(libusb_device_handle *devh, char dat[]) {
	// rc = libusb_control_transfer(devh, 0x21, 0x9, 0x02b3, 0x1, dat, sizeof(dat), 0);
	// printf("rc=%d (%s)\n", rc, libusb_error_name(rc));
	printf("pktsize=%d\n", sizeof(dat));
	return (libusb_control_transfer(devh, 0x21, 0x9, 0x02b3, 0x1, dat, sizeof(dat), 0));
}

int main(void) {
	printf("\n---===[ musg2tool ]===---\n");

	printf("Initializing libusb...\n");
	rc = libusb_init(&context);
	assert(rc == 0);

	libusb_device *dev;
	libusb_device_handle *devh;

	printf("Searching for device...\n");
	dev = searchForDevice();
	if (!dev) {
		printf("Could not find MUSG-02 mouse. Exiting...\n");
		return 1;
	}

	printf("Contacting device...\n");
	libusb_open(dev, &devh);
	struct libusb_device_descriptor desc = { 0 };
	libusb_get_device_descriptor(dev, &desc);
	if (!devh) {
		printf("Could not find MUSG-02 mouse. Exiting...\n");
		return 1;
	}
	printf("Opened: %04x:%04x\n", desc.idVendor, desc.idProduct);

	int kernelDriverAttached = libusb_kernel_driver_active(devh, IFACE);
	if (kernelDriverAttached != 0) {
		printf("Detatching kernel driver... (if crash, retry as superuser)\n");
		libusb_detach_kernel_driver(devh, IFACE);
	}
	printf("Claiming interface...\n");
	libusb_claim_interface(devh, IFACE);

	printf("Transferring data...\n");

	char data[] = { 0xb3, 0x03, 0x05, 0xfa, 0x55, 0xaa, 0x55, 0x00 };
	submitControlRq(devh, data);

	sleep(1);

	printf("Done transferring data...\n");

	printf("Releasing device...\n");
	libusb_release_interface(devh, IFACE);
	if (kernelDriverAttached != 0) {
		libusb_attach_kernel_driver(devh, IFACE);
	}
	libusb_close(devh);

	printf("Exiting libusb...\n");
	libusb_free_device_list(list, count);
	libusb_exit(context);

	return 0;
}
