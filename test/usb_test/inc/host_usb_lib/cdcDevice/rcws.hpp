/*
 * File: rcws.h
 * Created Date: 2023-03-31
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday April 11th 2023 12:08:40 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */
#pragma once

#include <libserial/SerialPort.h>
#include <libudev.h>
#include <libusb-1.0/libusb.h>
#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <host_usb_lib/command/command.hpp>
#include <host_usb_lib/parser/parser.hpp>

#include "msg_generator.hpp"

namespace lra::usb_lib {

/**
 *
 */
typedef struct {
  std::string path;
  std::string desc;
  std::string pid;
  std::string vid;
  std::string busnum;
  std::string devnum;
  std::string serialnum;
  std::string manufacturer;
} RCWSInfo;

class RCWS {
 public:
  explicit RCWS() { _RegisterAllCommands(); }
  bool Open() {
    if (rcws_info_.path.empty()) {
      auto rcws_candidates = FindAllRcws();

      if (rcws_candidates.empty()) {
        Log("No RCWS found\n");
        return false;
      }

      auto target_rcws = ChooseRcws(rcws_candidates);

      if (target_rcws.path.empty()) {
        Log("Invalid RCWS target: devnode path is empty\n");
        return false;
      }
      rcws_info_ = target_rcws;
    }

    serial_io_.Open(rcws_info_.path);

    if (!serial_io_.IsOpen()) return false;

    serial_io_.SetDTR(true);
    return true;
  }

  bool Close() {
    // unset DTR
    serial_io_.SetDTR(false);
    serial_io_.Close();
    return !serial_io_.IsOpen();
  }

  /**
   * This is a blocking user input function
   */
  RCWSInfo ChooseRcws(std::vector<RCWSInfo> info) {
    if (info.empty()) {
      Log("ChooseRcws: empty info vector, skipping the user selection part\n");
      return {};
    }
  }

  /**
   * Ref: https://stackoverflow.com/a/49207881
   */
  std::vector<RCWSInfo> FindAllRcws() {
    std::vector<RCWSInfo> rcws_info;

    try {
      struct udev* udev = udev_new();
      if (!udev) {
        throw std::runtime_error("failed to new udev\n");
      }

      struct udev_enumerate* enumerate = udev_enumerate_new(udev);
      udev_enumerate_add_match_subsystem(enumerate, "tty");
      udev_enumerate_scan_devices(enumerate);

      struct udev_list_entry* devices =
          udev_enumerate_get_list_entry(enumerate);
      struct udev_list_entry* entry;

      udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* tty_device =
            udev_device_new_from_syspath(udev, path);

        // Get the device file (e.g., /dev/ttyUSB0 or /dev/ttyACM0)
        const char* pdevnode = udev_device_get_devnode(tty_device);

        // TODO: https://stackoverflow.com/a/49207849

        /* Get the parent of tty device -> usbbus device (e.g.
         /dev/bus/usb/001/002) */
        struct udev_device* usbbus_device =
            udev_device_get_parent_with_subsystem_devtype(tty_device, "usb",
                                                          "usb_device");
        if (!usbbus_device) {
          udev_device_unref(tty_device);
          continue;
        }

        // Get PID, VID, busnum and devnum
        // use udevadm info --attribute-walk --path=/sys/bus/usb/devices/usb1 to
        // get ATTR{}
        const char* pvid =
            udev_device_get_sysattr_value(usbbus_device, "idVendor");
        const char* ppid =
            udev_device_get_sysattr_value(usbbus_device, "idProduct");
        const char* pbusnum =
            udev_device_get_sysattr_value(usbbus_device, "busnum");
        const char* pdevnum =
            udev_device_get_sysattr_value(usbbus_device, "devnum");
        const char* pserialnum =
            udev_device_get_sysattr_value(usbbus_device, "serial");

        // Get descriptor
        // std::string descriptor = GetDevDescriptor(pbusnum, pdevnum);
        const char* pdescriptor =
            udev_device_get_sysattr_value(usbbus_device, "product");

        const char* pmanufacturer =
            udev_device_get_sysattr_value(usbbus_device, "manufacturer");

        RCWSInfo info = {.path{safe_string(pdevnode)},
                         // .desc{descriptor},
                         .desc{safe_string(pdescriptor)},
                         .pid{safe_string(ppid)},
                         .vid{safe_string(pvid)},
                         .busnum{safe_string(pbusnum)},
                         .devnum{safe_string(pdevnum)},
                         .serialnum{safe_string(pserialnum)},
                         .manufacturer{safe_string(pmanufacturer)}};

        rcws_info.push_back(info);

        udev_device_unref(tty_device);
        /* Only device created by new related function should do unref.
        udev_device_get_parent_with_subsystem_devtype doesn't increase the ref
        counter. Therefore, you don't need to do
        udev_device_unref(usbbus_device). */
      }

      udev_enumerate_unref(enumerate);
      udev_unref(udev);

    } catch (const std::runtime_error& e) {
      Log("run time error: {}\n", e.what());
    }

    return rcws_info;
  }

  void DevInit() {}

  void DevReset(uint8_t dev_index) {}

  void DevPwmCmd() {}

  std::vector<RCWSCmdType> command_vec_;

 private:
  void WriteRcwsMsg(uint8_t msg_type, std::vector<uint8_t> data) {
    auto msg = msg_generator_.Generate(msg_type, data);
    if (serial_io_.IsOpen()) {
      serial_io_.DrainWriteBuffer();
      serial_io_.Write(msg);
    }
    return;
  }
  std::vector<uint8_t> ReadRcwsMsg() {
    std::vector<uint8_t> body;
    std::vector<uint8_t> header;
    uint16_t data_len;

    if (mode_ == LRA_USB_DATA_MODE)
      body.reserve(128);
    else
      body.reserve(32);

    if (!serial_io_.IsOpen()) return {};

    serial_io_.Read(header, 3, 0);
    data_len = header[1] << 8 | header[2];

    serial_io_.Read(body, data_len, 0);
    header.insert(header.end(), body.begin(), body.end());

    return header;
  }

  template <typename... Args>
  void Log(fmt::format_string<Args...> format_str, Args&&... args) {
    // TODO: change your logger here
    fmt::print(format_str, std::forward<Args>(args)...);
  }

  template <class C>
  inline std::basic_string<C> safe_string(const C* input) {
    if (!input) return std::basic_string<C>();
    return std::basic_string<C>(input);
  }

  /**
   * Ref: https://stackoverflow.com/a/73260280
   */
  std::string GetDevDescriptor(const char* pbusnum, const char* pdevnum) {
    libusb_context* ctx;
    libusb_device** dev_list;
    libusb_device_handle* handle;
    ssize_t num_devs, i;

    std::string desc{"Failed to get device descriptor"};

    if (!pbusnum || !pdevnum) {
      Log("pbusnum or pdevnum is NULL, aborting GetDevDescriptor\n");
      return desc;
    }

    uint8_t target_busnum = std::stoi(pbusnum);
    uint8_t target_devnum = std::stoi(pdevnum);

    if (libusb_init(&ctx) < 0) {
      Log("Libusb init failed, aborting GetDevDescriptor\n");
      return desc;
    }

    // get all usb devices
    num_devs = libusb_get_device_list(ctx, &dev_list);
    if (num_devs < 0) {
      Log("Error getting device list, aborting GetDevDescriptor\n");
      libusb_exit(ctx);
      return desc;
    }

    if (num_devs == 0) {
      Log("No USB device found, aborting GetDevDescriptor\n");
      libusb_exit(ctx);
      return desc;
    }

    libusb_device* device = nullptr;
    for (i = 0; i < num_devs; ++i) {
      if (libusb_get_bus_number(dev_list[i]) == target_busnum &&
          libusb_get_device_address(dev_list[i]) == target_devnum) {
        device = dev_list[i];
        break;
      }
    }

    if (device == nullptr) {
      Log("No matching devices found, busnum:{}, devnum{}, aborting "
          "GetDevDescriptor\n",
          target_busnum, target_devnum);
      libusb_free_device_list(dev_list, 1);
      libusb_exit(ctx);
      return desc;
    }

    if (libusb_open(device, &handle) < 0) {
      Log("Error opening device, aborting GetDevDescriptor\n");
      libusb_free_device_list(dev_list, 1);
      libusb_exit(ctx);
      return desc;
    }

    struct libusb_device_descriptor dev_desc;
    if (libusb_get_device_descriptor(device, &dev_desc) < 0) {
      Log("Error getting device descriptor, aborting GetDevDescriptor\n");
      libusb_close(handle);
      libusb_free_device_list(dev_list, 1);
      libusb_exit(ctx);
      exit(1);
    }

    unsigned char product_name[256];
    libusb_get_string_descriptor_ascii(handle, dev_desc.iProduct, product_name,
                                       sizeof(product_name));

    libusb_close(handle);
    libusb_free_device_list(dev_list, 1);
    libusb_exit(ctx);
    return std::string(reinterpret_cast<char*>(product_name));
  }

  void _RegisterAllCommands() {}

  uint8_t mode_{LRA_USB_WAIT_FOR_INIT_MODE};
  RCWSMsgGenerator msg_generator_;
  RCWSParser parser_;
  // thread read_thread_;
  LibSerial::SerialPort serial_io_;
  RCWSInfo rcws_info_{};
};
}  // namespace lra::usb_lib
