#include <cassert>
#include <cstdio>
#include <libusb-1.0/libusb.h>
// g++ serial_read_usb.cpp -o serial_test -lusb-1.0
// https://blog.csdn.net/FlayHigherGT/article/details/90245349

int main() {
    libusb_context *context = NULL;
    libusb_device **list = NULL;
    int rc = 0;
    ssize_t count = 0;

    rc = libusb_init(&context);
    assert(rc == 0);

    libusb_get_string_descriptor_ascii();

    count = libusb_get_device_list(context, &list);
    assert(count > 0);

    printf("Count: %ld\n", count);

    for (size_t idx = 0; idx < count; ++idx) {
        libusb_device *device = list[idx];
        libusb_device_descriptor desc = {0};

        rc = libusb_get_device_descriptor(device, &desc);
        assert(rc == 0);
        printf("\n");
        printf("VID: %04x\n", desc.idVendor);
        printf("PID: %04x\n", desc.idProduct);
        printf("USBVersion: %03x\n", desc.bcdUSB);
    }

    libusb_free_device_list(list, count);
    libusb_exit(context);
}