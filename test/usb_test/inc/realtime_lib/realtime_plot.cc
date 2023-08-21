/*
 * File: realtime.cc
 * Created Date: 2023-08-21
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday August 21st 2023 12:03:09 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include "realtime_plot.h"

#include <host_usb_lib/logger/logger.h>

#include <fstream>
#include <string>

namespace lra::realtime_plot {
bool isRunningInWSL() {
  std::ifstream in("/proc/version");
  std::string content;
  getline(in, content);
  in.close();
  return content.find("Microsoft") != std::string::npos ||
         content.find("WSL") != std::string::npos;
}

std::string createPipe() {}

};  // namespace lra::realtime_plot
