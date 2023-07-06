/*
 * File: rcws.h
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:33:04 pm
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

#include <bit>
#include <chrono>
#include <cstdint>
#include <regex>
#include <thread>

// rcws libs
#include <host_usb_lib/logger/logger.h>
#include <host_usb_lib/parser/rcws_parser.h>

#include <util/range_bound.hpp>

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
  explicit Rcws();
  ~Rcws();
  bool Open();
  bool Close();
  RcwsInfo ChooseRcws(std::vector<RcwsInfo> info, size_t index);
  void PrintRcwsPwmInfo(const RcwsPwmInfo& info);
  void PrintSelfInfo();
  void UserCallExit();
  void PrintAllRcwsInfo(std::vector<RcwsInfo> infos);
  /**
   * Ref: https://stackoverflow.com/a/49207881
   */
  std::vector<RcwsInfo> FindAllRcws();
  void UpdateAccFileHandle(FILE* handle);
  void UpdatePwmFileHandle(FILE* handle);
  FILE* GetAccFileHandle();
  FILE* GetPwmFileHandle();
  void UpdateAccFileName(std::string name);
  void UpdatePwmFileName(std::string name);
  std::string GetAccFileName();
  std::string GetPwmFileName();
  std::string GetNextFileName(const std::string& path,
                              const std::string& baseName);

  /* Device related functions */
  void DevInit();
  void DevReset(LRA_Device_Index_t dev_index);
  void DevSwitchMode(LRA_USB_Mode_t mode);
  void DevPwmCmd(const RcwsPwmInfo& info);

  /* Device IO */
  void DevSend(LRA_USB_OUT_Cmd_t cmd_type, std::vector<uint8_t> data);

  /* public vars */

  std::vector<RcwsCmdType> command_vec_;
  std::string data_path_{""};

  /**
   *  Private region
   */

 private:
  bool RangeCheck(const RcwsPwmInfo& info);
  /**
   * Convert RcwsPwmInfo to vec
   * example: https://godbolt.org/z/h85f55jGv
   *
   * @return when info is invalid or something goes wrong, return an empty
   * vector
   */
  std::vector<uint8_t> RcwsPwmInfoToVec(const RcwsPwmInfo& info);

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
  std::vector<uint8_t> ReadRcwsMsg();
  void PrintRcwsInfo(RcwsInfo& info);
  void ParseTask();

  /**
   * Ref: https://stackoverflow.com/a/73260280
   * Aborted
   */
  std::string GetDevDescriptor(const char* pbusnum, const char* pdevnum);
  void _RegisterAllCommands();

  uint8_t mode_{LRA_USB_WAIT_FOR_INIT_MODE};
  RcwsInfo rcws_info_{};
  std::vector<RcwsCmdType> registered_cmd_vec_;
  std::thread parser_thread_;
  FILE* acc_file_{nullptr};
  FILE* pwm_file_{nullptr};
  std::string current_acc_file_name_{""};
  std::string current_pwm_file_name_{""};

  /* External class */
  RcwsMsgGenerator msg_generator_;
  RcwsParser parser_;
  LibSerial::SerialPort serial_io_;

  bool read_thread_exit_{false};
  bool reset_stm32_flag_{false};
};

}  // namespace lra::usb_lib
