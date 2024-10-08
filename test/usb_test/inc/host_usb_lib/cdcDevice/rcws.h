/*
 * File: rcws.h
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday August 24th 2023 12:01:53 pm
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
#include <queue>
#include <regex>
#include <thread>

// rcws libs
#include <host_usb_lib/logger/logger.h>
#include <host_usb_lib/parser/rcws_parser.h>

#include <util_lib/range_bound.hpp>

#include "msg_generator.hpp"
#include "rcws_info.hpp"

namespace lra::usb_lib {

template <class C>
static inline std::basic_string<C> safe_string(const C* input) {
  if (!input) return std::basic_string<C>();
  return std::basic_string<C>(input);
}

class RCWS_IO_Exception : public std::exception {
 public:
  explicit RCWS_IO_Exception(const std::string& message);

  const char* what() const noexcept override;

 private:
  std::string errorMessage;
};

class Rcws {
 public:
  explicit Rcws();
  ~Rcws();
  bool Open();
  bool Close();
  RcwsInfo ChooseRcws(std::vector<RcwsInfo> info, size_t index);
  void PrintRcwsPwmInfo(const RcwsPwmInfo& info);
  void PrintSelfInfo();
  RcwsInfo GetRcwsInfo();
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
  int GetLibSerialFd();
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

  /* PwmCmdThread related */
  bool PwmCmdThreadRunning();
  void CleanPwmCmdQueue();
  void PwmCmdThreadClose();
  void PwmCmdSetRecursive(bool enable);
  void StartPwmCmdThread(std::string csv_path);

  /* public vars */
  std::string pipe_name_;
  std::vector<RcwsCmdType> command_vec_;
  std::string data_path_{""};

  /**
   *  Private region
   */

 private:
  bool RangeCheck(const RcwsPwmInfo& info);
  /* TODO: add try catch */
  std::vector<uint8_t> ReadRcwsMsg();
  void PrintRcwsInfo(RcwsInfo& info);
  void ParseTask();
  void PwmCmdTask(std::string path);
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

  /**
   * Ref: https://stackoverflow.com/a/73260280
   * Aborted
   */
  std::string GetDevDescriptor(const char* pbusnum, const char* pdevnum);
  void _RegisterAllCommands();

  uint8_t mode_{LRA_USB_WAIT_FOR_INIT_MODE};
  RcwsInfo rcws_info_{};
  std::vector<RcwsCmdType> registered_cmd_vec_;

  FILE* acc_file_{nullptr};
  FILE* pwm_file_{nullptr};
  std::string current_acc_file_name_{""};
  std::string current_pwm_file_name_{""};

  /* ReadThread */
  std::thread parser_thread_;
  bool read_thread_exit_{false};

  /* PwmCmdThread */
  std::queue<RcwsPwmCmd> pwm_cmd_q_;
  std::thread pwm_cmd_thread_;
  bool pwm_cmd_thread_exit_{false};
  bool recursive_flag_{false};

  /* External class */
  RcwsMsgGenerator msg_generator_;
  RcwsParser parser_;
  LibSerial::SerialPort serial_io_;

  bool reset_stm32_flag_{false};
};

}  // namespace lra::usb_lib
