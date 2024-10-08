/*
 * File: rcws_info.hpp
 * Created Date: 2023-04-13
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday August 18th 2023 1:16:43 pm
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

#include <drv_stm_lib/lra_usb_defines.h>

#include <host_usb_lib/command/command_impl.hpp>
#include <host_usb_lib/command/function_info.hpp>
#include <string>
#include <variant>
#include <vector>

namespace lra::usb_lib {
typedef struct {
  std::string path;
  std::string desc;
  std::string pid;
  std::string vid;
  std::string busnum;
  std::string devnum;
  std::string serialnum;
  std::string manufacturer;
} RcwsInfo;

typedef struct {
  float amp;
  float freq;
} PwmInfo;

typedef struct {
  PwmInfo x;
  PwmInfo y;
  PwmInfo z;
} RcwsPwmInfo;

typedef struct {
  float t_ms;
  RcwsPwmInfo info;
} RcwsPwmCmd;

using RcwsCmdType = std::variant<
    std::monostate, Command<FuncInfo<void>>, Command<FuncInfo<void, int>>,
    Command<FuncInfo<void, std::vector<RcwsInfo>>>,
    Command<FuncInfo<void, LRA_Device_Index_t>>,
    Command<FuncInfo<void, LRA_USB_Mode_t>>, Command<FuncInfo<bool>>,
    Command<FuncInfo<int, int, int>>, Command<FuncInfo<std::vector<RcwsInfo>>>,
    Command<FuncInfo<RcwsInfo, std::vector<RcwsInfo>, int>>,
    Command<FuncInfo<RcwsInfo, std::vector<RcwsInfo>, size_t>>>;
}  // namespace lra::usb_lib
