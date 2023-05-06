/*
 * File: rcws.hpp
 * Created Date: 2023-03-31
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday May 5th 2023 2:08:39 am
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

#include <chrono>
#include <cstdint>
#include <host_usb_lib/logger/logger.hpp>
#include <host_usb_lib/parser/rcws_parser.hpp>
#include <thread>

#include "msg_generator.hpp"
#include "rcws_info.hpp"

namespace lra::usb_lib {

template <class C>
static inline std::basic_string<C> safe_string(const C* input) {
  if (!input) return std::basic_string<C>();
  return std::basic_string<C>(input);
}

class Rcws {
 public:
  explicit Rcws() {
    _RegisterAllCommands();
    parser_.RegisterDevice(this);
    parser_thread_ = std::thread(&Rcws::ParseTask, this);
  }

  ~Rcws() {
    read_thread_exit_ = true;
    parser_thread_.join();
  }

  // TODO: rewrite open, chooseRcws
  bool Open() {
    // reset serial_port
    if (reset_stm32_flag_) {
      Log(fg(fmt::terminal_color::bright_magenta),
          "Warning: Rcws just be reset. Please wait for 10 seconds to "
          "reconnect\n");
      serial_io_ = LibSerial::SerialPort();
      reset_stm32_flag_ = false;
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    if (serial_io_.IsOpen()) {
      Log(fg(fmt::terminal_color::bright_magenta), "Port already opened\n");
      return false;
    }

    if (rcws_info_.path.empty()) {
      Log(fg(fmt::terminal_color::bright_red),
          "Invalid open! Please choose rcws instance first\n");
      return false;
    }

    serial_io_.Open(rcws_info_.path);

    if (!serial_io_.IsOpen()) {
      Log("Serial port path: {} open failed\n", rcws_info_.path);
      return false;
    }

    serial_io_.SetDTR(true);
    Log(fg(fmt::terminal_color::bright_green), "Serial port open success\n");
    return true;
  }

  bool Close() {
    // unset DTR
    bool isopen = serial_io_.IsOpen();
    if (!isopen) {
      Log(fg(fmt::terminal_color::bright_magenta),
          "Serial Port already closed\n");
      return false;
    }
    serial_io_.SetDTR(false);
    serial_io_.Close();
    Log(fg(fmt::terminal_color::bright_green), "Serial port closed\n");
    return !serial_io_.IsOpen();
  }

  /**
   * This is a blocking user input function
   */
  RcwsInfo ChooseRcws(std::vector<RcwsInfo> info, size_t index) {
    if (info.empty() || (index >= info.size())) {
      Log("ChooseRcws: empty info vector, skipping the user selection part\n");
      rcws_info_ = {};
    } else {
      rcws_info_ = info[index];
    }

    return rcws_info_;
  }

  void PrintAllRcwsInfo(std::vector<RcwsInfo> infos) {
    Log("\n");
    Log("All RCWS detected:\n\n");
    int index = 0;
    for (auto& info : infos) {
      Log("index -- {}\n", index++);
      PrintRcwsInfo(info);
    }
  }

  /**
   * Ref: https://stackoverflow.com/a/49207881
   */
  std::vector<RcwsInfo> FindAllRcws() {
    std::vector<RcwsInfo> rcws_info;

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

        RcwsInfo info = {.path{safe_string(pdevnode)},
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
      Log(fg(fmt::terminal_color::bright_red), "run time error: {}\n",
          e.what());
    }

    /* print all rcws */
    return rcws_info;
  }

  void DevInit() { WriteRcwsMsg(USB_OUT_CMD_INIT); }

  void DevReset(LRA_Device_Index_t dev_index) {
    WriteRcwsMsg(USB_OUT_CMD_RESET_DEVICE, (uint8_t)dev_index);
    if (dev_index == LRA_DEVICE_STM32) reset_stm32_flag_ = true;
  }

  void DevSend(LRA_USB_OUT_Cmd_t cmd_type, std::vector<uint8_t> data) {
    WriteRcwsMsg(cmd_type, data);
  }

  void DevPwmCmd() {}

  void DevSwitchMode(LRA_USB_Mode_t mode) {
    WriteRcwsMsg(USB_OUT_CMD_SWITCH_MODE, (uint8_t)mode);
  }

  void PrintSelfInfo() { PrintRcwsInfo(rcws_info_); }

  void UserCallExit() { read_thread_exit_ = true; }

  std::vector<RcwsCmdType> command_vec_;

  /**
   *  Private region
   */

 private:
  template <typename... Args>
  void WriteRcwsMsg(LRA_USB_OUT_Cmd_t msg_type, Args&&... args) {
    auto msg = msg_generator_.Generate(msg_type, std::forward<Args>(args)...);

    if (serial_io_.IsOpen()) {
      serial_io_.DrainWriteBuffer();
      serial_io_.Write(msg);
    } else {
      Log(fg(fmt::terminal_color::bright_red), "Serial port is not open yet\n");
    }
  }

  /* TODO: add try catch */
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

    try {
      serial_io_.Read(body, data_len, 1000);
      header.insert(header.end(), body.begin(), body.end());
      return header;
    } catch (const std::exception& e) {
      Log("ReadRcwsMsg exception: {}\n", e.what());
      return {};
    }
  }

  void PrintRcwsInfo(RcwsInfo& info) {
    Log("{{\n");
    Log("\tPath: {}\n", info.path);
    Log("\tManufacturer: {}\n", info.manufacturer);
    Log("\tProduct String: {}\n", info.desc);
    Log("\tPID: {}\n", info.pid);
    Log("\tVID: {}\n", info.vid);
    Log("\tSerial Number: {}\n", info.serialnum);
    Log("\tBus Number: {}\n", info.busnum);
    Log("\tDevice Number: {}\n", info.devnum);
    Log("}}\n");
    Log("\n");
  }

  void ParseTask() {
    while (!read_thread_exit_) {
      if (serial_io_.IsOpen() && serial_io_.IsDataAvailable()) {
        auto msg = ReadRcwsMsg();
        parser_.Parse(msg);
      }
    }
  }

  /**
   * Ref: https://stackoverflow.com/a/73260280
   * Aborted
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

  void _RegisterAllCommands() {
    /* DevReset */
    auto fi_resetdev = make_func_info(&Rcws::DevReset, this, LRA_DEVICE_ALL);
    auto cmd_resetdev = Command("Rcws::DevReset", "Reset device given by user",
                                std::move(fi_resetdev));
    registered_cmd_vec_.push_back(cmd_resetdev);

    /* DevInit */
    auto fi_initdev = make_func_info(&Rcws::DevInit, this);
    auto cmd_initdev =
        Command("Rcws::DevInit", "Init RCWS (DRV_STM)", std::move(fi_initdev));
    registered_cmd_vec_.push_back(cmd_initdev);

    /* DevSwitchMode */
    auto fi_switchmode =
        make_func_info(&Rcws::DevSwitchMode, this, LRA_USB_CRTL_MODE);
    auto cmd_switchmode = Command("Rcws::DevSwitchMode", "Switch RCWS Mode",
                                  std::move(fi_switchmode));
    registered_cmd_vec_.push_back(cmd_switchmode);

    /* --------------------------------------------------------------------- */

    /* Open */
    auto fi_open = make_func_info(&Rcws::Open, this);
    auto cmd_open =
        Command("Rcws::Open", "Open serial port to communicate to RCWS",
                std::move(fi_open));
    registered_cmd_vec_.push_back(cmd_open);

    /* Close */
    auto fi_close = make_func_info(&Rcws::Close, this);
    auto cmd_close =
        Command("Rcws::Close", "Close serial port", std::move(fi_close));
    registered_cmd_vec_.push_back(cmd_close);

    /* ChooseRcws */
    auto fi_choosercws = make_func_info(&Rcws::ChooseRcws, this, {}, (size_t)0);
    auto cmd_choosercws =
        Command("Rcws::ChooseRcws",
                "Select RCWS instance from given std::vector<RcwsInfo>",
                std::move(fi_choosercws));
    registered_cmd_vec_.push_back(cmd_choosercws);

    /* FindAllRcws */
    auto fi_findallrcws = make_func_info(&Rcws::FindAllRcws, this);
    auto cmd_findallrcws = Command(
        "Rcws::FindAllRcws", "Find all mounted Rcws in current OS [Linux]",
        std::move(fi_findallrcws));
    registered_cmd_vec_.push_back(cmd_findallrcws);

    /* PrintAllRcwsInfo */
    auto fi_printallrcwsinfo =
        make_func_info(&Rcws::PrintAllRcwsInfo, this, {});
    auto cmd_printallrcwsinfo =
        Command("Rcws::PrintAllRcwsInfo", "Print all detected RCWS information",
                std::move(fi_printallrcwsinfo));
    registered_cmd_vec_.push_back(cmd_printallrcwsinfo);
  }

  uint8_t mode_{LRA_USB_WAIT_FOR_INIT_MODE};
  RcwsInfo rcws_info_{};
  std::vector<RcwsCmdType> registered_cmd_vec_;
  std::thread parser_thread_;

  /* External class */
  RcwsMsgGenerator msg_generator_;
  RcwsParser parser_;
  LibSerial::SerialPort serial_io_;

  bool read_thread_exit_{false};
  bool reset_stm32_flag_{false};
};

}  // namespace lra::usb_lib
