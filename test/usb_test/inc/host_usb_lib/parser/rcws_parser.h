/*
 * File: rcws_parser.h
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:33:22 pm
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
#include <host_usb_lib/cdcDevice/rcws.h>
#include <host_usb_lib/logger/logger.h>

#include <host_usb_lib/command/command.hpp>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace lra::usb_lib {

class Rcws;

class RcwsParser {
 public:
  void Parse(const std::vector<uint8_t>& msg);
  void RegisterDevice(Rcws* prcws);
  void ParsePwmInData(const uint8_t* pdata, RcwsPwmInfo* info, float* sys_time);

 private:
  Rcws* prcws_{nullptr};
};

}  // namespace lra::usb_lib