/*
 * File: logger.cc
 * Created Date: 2023-07-06
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:50:00 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */
#include "logger.h"
namespace lra::usb_lib {

std::mutex stdout_print_mutex{};

}  // namespace lra::usb_lib
