/*
 * File: detect_usb.cc
 * Created Date: 2023-04-09
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday April 10th 2023 10:07:40 am
 *
 * Copyright (c) 2023 None
 *
 * Compile: g++ detect_usb.cc -g -o detect_usb -ludev -lusb-1.0
 *
 * Ref: https://stackoverflow.com/a/73260280
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <libudev.h>
#include <libusb-1.0/libusb.h>

#include <iostream>
int main() {
  struct udev *udev = udev_new();
  if (!udev) {
    std::cerr << "udev_new() failed" << std::endl;
    return 1;
  }

  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "tty");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *device = udev_device_new_from_syspath(udev, path);

    //// Get the parent tty device
    // struct udev_device *tty_device =
    //     udev_device_get_parent_with_subsystem_devtype(device, "usb",
    //                                                   "usb_device");
    // if (!tty_device) {
    //   udev_device_unref(device);
    //   continue;
    // }

    // find all tty device (filter virtual)
    std::string s1, s2;
    s1 = udev_device_get_devpath(device);
    s2 = "/devices/virtual/";
    if ((udev_device_get_sysnum(device) != NULL) &&
        s1.find(s2) == std::string::npos) {
      // Get the device file (e.g., /dev/ttyUSB0 or /dev/ttyACM0)
      const char *devnode = udev_device_get_devnode(device);

      // Print the device information
      std::cout << "Device: " << devnode << std::endl;
    }
    udev_device_unref(device);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);

  return 0;
}
