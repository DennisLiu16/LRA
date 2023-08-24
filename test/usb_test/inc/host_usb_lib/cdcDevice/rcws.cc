/*
 * File: rcws.cc
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday August 24th 2023 10:01:06 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <host_usb_lib/cdcDevice/rcws.h>
#include <libserial/SerialPort.h>
#include <libudev.h>
#include <libusb-1.0/libusb.h>

#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <regex>
#include <thread>

// rcws libs
#include <fft_lib/third_party/csv.h>
#include <host_usb_lib/logger/logger.h>
#include <host_usb_lib/parser/rcws_parser.h>

#include <util/range_bound.hpp>

#include "msg_generator.hpp"
#include "rcws_info.hpp"

namespace lra::usb_lib {
RCWS_IO_Exception::RCWS_IO_Exception(const std::string& message)
    : errorMessage(message) {}

const char* RCWS_IO_Exception::what() const noexcept {
  return errorMessage.c_str();
}

Rcws::Rcws() {
  _RegisterAllCommands();
  parser_.RegisterDevice(this);
  parser_thread_ = std::thread(&Rcws::ParseTask, this);
}

Rcws::~Rcws() {
  read_thread_exit_ = true;
  pwm_cmd_thread_exit_ = true;
  if (parser_thread_.joinable()) {
    parser_thread_.join();
  }
  if (pwm_cmd_thread_.joinable()) {
    pwm_cmd_thread_.join();
  }
}

// TODO: rewrite open, chooseRcws
bool Rcws::Open() {
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

  Log("Going to open :{}\n", rcws_info_.path);

  serial_io_.Open(rcws_info_.path);

  if (!serial_io_.IsOpen()) {
    Log("Serial port path: {} open failed\n", rcws_info_.path);
    return false;
  }

  serial_io_.SetDTR(true);
  Log(fg(fmt::terminal_color::bright_green), "Serial port open success\n");
  return true;
}

bool Rcws::Close() {
  bool isopen;
  try {
    // unset DTR
    isopen = serial_io_.IsOpen();
    if (!isopen) {
      Log(fg(fmt::terminal_color::bright_magenta),
          "Serial Port already closed\n");
      return false;
    }

    serial_io_.SetDTR(false);
    serial_io_.Close();

    Log(fg(fmt::terminal_color::bright_green), "Serial port closed\n");

    return !serial_io_.IsOpen();

  } catch (const std::exception& e) {
    Log(fg(fmt::terminal_color::bright_red),
        "Exception: Serial port closed failed.\nCreate new serial port to "
        "enable next open.\n");
    serial_io_ = LibSerial::SerialPort();
    return false;
  }
}

/**
 * This is a blocking user input function
 */
RcwsInfo Rcws::ChooseRcws(std::vector<RcwsInfo> info, size_t index) {
  if (info.empty() || (index >= info.size())) {
    Log("ChooseRcws: empty info vector, skipping the user selection part\n");
    rcws_info_ = {};
  } else {
    rcws_info_ = info[index];
  }

  return rcws_info_;
}

void Rcws::PrintRcwsPwmInfo(const RcwsPwmInfo& info) {
  Log("X amp: {}\n", info.x.amp);
  Log("X freq: {}\n", info.x.freq);
  Log("Y amp: {}\n", info.y.amp);
  Log("Y freq: {}\n", info.y.freq);
  Log("Z amp: {}\n", info.z.amp);
  Log("Z freq: {}\n", info.z.freq);
}

void Rcws::PrintSelfInfo() { PrintRcwsInfo(rcws_info_); }

RcwsInfo Rcws::GetRcwsInfo() { return rcws_info_; }

void Rcws::UserCallExit() { read_thread_exit_ = true; }

void Rcws::PrintAllRcwsInfo(std::vector<RcwsInfo> infos) {
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
std::vector<RcwsInfo> Rcws::FindAllRcws() {
  std::vector<RcwsInfo> rcws_info;

  try {
    struct udev* udev = udev_new();
    if (!udev) {
      throw std::runtime_error("failed to new udev\n");
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
      const char* path = udev_list_entry_get_name(entry);
      struct udev_device* tty_device = udev_device_new_from_syspath(udev, path);

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
    Log(fg(fmt::terminal_color::bright_red), "run time error: {}\n", e.what());
  }

  /* print all rcws */
  return rcws_info;
}

void Rcws::UpdateAccFileHandle(FILE* handle) { acc_file_ = handle; }

void Rcws::UpdatePwmFileHandle(FILE* handle) { pwm_file_ = handle; }

FILE* Rcws::GetAccFileHandle() { return acc_file_; }

FILE* Rcws::GetPwmFileHandle() { return pwm_file_; }

void Rcws::UpdateAccFileName(std::string name) {
  current_acc_file_name_ = name;
}

void Rcws::UpdatePwmFileName(std::string name) {
  current_pwm_file_name_ = name;
}

int Rcws::GetLibSerialFd() { return serial_io_.GetFileDescriptor(); }

std::string Rcws::GetAccFileName() { return current_acc_file_name_; }

std::string Rcws::GetPwmFileName() { return current_pwm_file_name_; }

std::string Rcws::GetNextFileName(const std::string& path,
                                  const std::string& baseName) {
  std::regex baseNamePattern(baseName + "_(\\d+)\\.txt");

  int maxIndex = 0;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    std::smatch match;
    std::string candidate = entry.path().filename().string();
    if (std::regex_match(candidate, match, baseNamePattern)) {
      int index = std::stoi(match[1].str());
      maxIndex = std::max(maxIndex, index);
    }
  }

  return baseName + "_" + std::to_string(maxIndex + 1) + ".txt";
}

/* Device related functions */
void Rcws::DevInit() { WriteRcwsMsg(USB_OUT_CMD_INIT); }

void Rcws::DevReset(LRA_Device_Index_t dev_index) {
  WriteRcwsMsg(USB_OUT_CMD_RESET_DEVICE, (uint8_t)dev_index);
  if (dev_index == LRA_DEVICE_STM32) reset_stm32_flag_ = true;
}

void Rcws::DevSwitchMode(LRA_USB_Mode_t mode) {
  WriteRcwsMsg(USB_OUT_CMD_SWITCH_MODE, (uint8_t)mode);
}

void Rcws::DevPwmCmd(const RcwsPwmInfo& info) {
  /* create a uint8_t vector with correct data (no \r\n) */
  std::vector<uint8_t> data = RcwsPwmInfoToVec(info);

  /* format data */

  /* TODO: optimize buffer generation */
  WriteRcwsMsg(USB_OUT_CMD_UPDATE_PWM, data);
}

/* Device IO */
void Rcws::DevSend(LRA_USB_OUT_Cmd_t cmd_type, std::vector<uint8_t> data) {
  WriteRcwsMsg(cmd_type, data);
}

/**
 *  Private region
 */

bool Rcws::RangeCheck(const RcwsPwmInfo& info) {
  // exclusive max value
  constexpr float ex_max_amp = 1000;
  constexpr float ex_max_freq = 10.0;

  // exclusive min value
  constexpr float ex_min_amp = 0;
  constexpr float ex_min_freq = 1.0;

  lra::util::Range ampRange(ex_min_amp, ex_max_amp);
  lra::util::Range freqRange(ex_min_freq, ex_max_freq);

  bool ampInRange = ampRange.isWithinRange(info.x.amp) &&
                    ampRange.isWithinRange(info.y.amp) &&
                    ampRange.isWithinRange(info.z.amp);

  bool freqInRange = freqRange.isWithinRange(info.x.freq) &&
                     freqRange.isWithinRange(info.y.freq) &&
                     freqRange.isWithinRange(info.z.freq);

  return ampInRange && freqInRange;
}

/**
 * Convert RcwsPwmInfo to vec
 * example: https://godbolt.org/z/h85f55jGv
 *
 * @return when info is invalid or something goes wrong, return an empty
 * vector
 */
std::vector<uint8_t> Rcws::RcwsPwmInfoToVec(const RcwsPwmInfo& info) {
  /* Range check  */
  bool range_ok = RangeCheck(info);
  if (range_ok != true) return {};

  constexpr size_t buf_len = sizeof(RcwsPwmInfo) / sizeof(float);
  constexpr size_t para_num_per_axis = sizeof(PwmInfo) / sizeof(float);
  float data[buf_len];

  /* RcwsPwmInfo is POD data */
  memcpy(data, &info, sizeof(RcwsPwmInfo));

  // 4 bytes amp + 4 bytes freq +  parameter delimiter',' + axis delimiter';'
  // => 10 bytes for an axis, so len == 30 for 3 axes
  std::vector<uint8_t> data_vec;
  data_vec.reserve(30);

  // Set cursor to point to float array data
  uint8_t* cursor = reinterpret_cast<uint8_t*>(data);

  auto is_end_of_axis = [para_num_per_axis](size_t index) {
    return (index + 1) % para_num_per_axis == 0;
  };

  for (size_t i = 0; i < buf_len; ++i) {
    // Write float bytes in proper order, default little endian
    for (size_t byte_idx = 0; byte_idx < sizeof(float); ++byte_idx) {
      if constexpr (std::endian::native == std::endian::little) {
        data_vec.push_back(cursor[i * sizeof(float) + byte_idx]);
      } else {
        data_vec.push_back(cursor[(i + 1) * sizeof(float) - byte_idx - 1]);
      }
    }

    // Add delimiter symbols
    if (is_end_of_axis(i)) {
      data_vec.push_back(';');
    } else {
      // seperate different parameters
      data_vec.push_back(',');
    }
  }

  return data_vec;
}

/* TODO: add try catch */
std::vector<uint8_t> Rcws::ReadRcwsMsg() {
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
    serial_io_.Read(body, data_len, 25);
    header.insert(header.end(), body.begin(), body.end());
    return header;
  } catch (const std::exception& e) {
    // Log("ReadRcwsMsg exception: {}\n", e.what());
    return {};
  }
}

void Rcws::PrintRcwsInfo(RcwsInfo& info) {
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

void Rcws::ParseTask() {
  while (!read_thread_exit_) {
    if (serial_io_.IsOpen() && serial_io_.IsDataAvailable()) {
      auto msg = ReadRcwsMsg();
      parser_.Parse(msg);
    }
  }
}

void Rcws::PwmCmdTask(std::string path) {
  /* load csv file */
  io::CSVReader<7> csv(path);

  /* read data */
  RcwsPwmCmd cmd;
  float current_time = -1;
  float time_bias = -1;
  bool the_first_flag = true;

  // the unit of current time is second
  while (csv.read_row(current_time, cmd.info.x.amp, cmd.info.x.freq,
                      cmd.info.y.amp, cmd.info.y.freq, cmd.info.z.amp,
                      cmd.info.z.freq)) {
    if (the_first_flag) {
      time_bias = current_time;
      the_first_flag = false;
      cmd.t_ms = 0;
    }

    else
      cmd.t_ms = (current_time - time_bias) * 1000;

    // push to queue
    pwm_cmd_q_.push(cmd);
  }

  const size_t q_total_len = pwm_cmd_q_.size();
  int last_percentage_print = -1;

  /* print length */
  Log("\n"
      "Load pwm csv file completed\n"
      "Size:{} \n",
      pwm_cmd_q_.size());

  Log(fg(fmt::terminal_color::bright_blue), "Start to simulate\n");

  /* init time vars */
  auto start_timepoint = std::chrono::high_resolution_clock::now();

  /* push into queue */
  while (!pwm_cmd_q_.empty() && !pwm_cmd_thread_exit_) {
    auto next_pwm_cmd = pwm_cmd_q_.front();

    // wait for time interval expired, spin lock
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - start_timepoint)
               .count() < next_pwm_cmd.t_ms) {
    }

    try {
      DevPwmCmd(next_pwm_cmd.info);
    } catch (const std::exception& e) {
      Log(fg(fmt::terminal_color::bright_red),
          "Exception: PWM cmd to RCWS failed\n");
      pwm_cmd_thread_exit_ = true;
    }

    if (recursive_flag_) {
      pwm_cmd_q_.push(next_pwm_cmd);
    } else {
      int percentage = (pwm_cmd_q_.size() * 100) / q_total_len;

      if (percentage % 5 == 0 && percentage != last_percentage_print) {
        Log(fg(fmt::terminal_color::bright_blue), "Pwm cmd remain {} %\n",
            percentage);
        last_percentage_print = percentage;
      }
    }

    // pop
    pwm_cmd_q_.pop();
  }

  /* TODO: stop vibration */
  RcwsPwmInfo _stop_info = {.x = {.amp = 500, .freq = 5},
                            .y = {.amp = 500, .freq = 5},
                            .z = {.amp = 500, .freq = 5}

  };

  try {
    DevPwmCmd(_stop_info);
  } catch (const std::exception& e) {
    Log(fg(fmt::terminal_color::bright_red),
        "Exception: PWM ended cmd transmit failed\n");
    pwm_cmd_thread_exit_ = true;
  }

  Log(fg(fmt::terminal_color::bright_blue), "Pwm task close\n");
}

bool Rcws::PwmCmdThreadRunning() { return pwm_cmd_thread_exit_; }

void Rcws::CleanPwmCmdQueue() {
  std::queue<RcwsPwmCmd> empty;
  pwm_cmd_q_.swap(empty);
}

void Rcws::PwmCmdThreadClose() {
  pwm_cmd_thread_exit_ = true;

  /* if thread is not detach */
  if (pwm_cmd_thread_.joinable()) {
    pwm_cmd_thread_.join();
  }
}

void Rcws::PwmCmdSetRecursive(bool enable) { recursive_flag_ = enable; }

void Rcws::StartPwmCmdThread(std::string csv_path) {
  if (Rcws::PwmCmdThreadRunning()) return;
  pwm_cmd_thread_ = std::thread(&Rcws::PwmCmdTask, this, csv_path);
  pwm_cmd_thread_.detach();
}

/**
 * Ref: https://stackoverflow.com/a/73260280
 * Aborted
 */
std::string Rcws::GetDevDescriptor(const char* pbusnum, const char* pdevnum) {
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

void Rcws::_RegisterAllCommands() {
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
  auto cmd_findallrcws = Command("Rcws::FindAllRcws",
                                 "Find all mounted Rcws in current OS [Linux]",
                                 std::move(fi_findallrcws));
  registered_cmd_vec_.push_back(cmd_findallrcws);

  /* PrintAllRcwsInfo */
  auto fi_printallrcwsinfo = make_func_info(&Rcws::PrintAllRcwsInfo, this, {});
  auto cmd_printallrcwsinfo =
      Command("Rcws::PrintAllRcwsInfo", "Print all detected RCWS information",
              std::move(fi_printallrcwsinfo));
  registered_cmd_vec_.push_back(cmd_printallrcwsinfo);
}
}  // namespace lra::usb_lib
