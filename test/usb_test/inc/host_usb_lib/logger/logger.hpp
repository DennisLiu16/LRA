/*
 * File: logger.hpp
 * Created Date: 2023-04-15
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Wednesday April 19th 2023 7:03:56 pm
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

#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/fmt.h>

namespace lra::usb_lib {

/* remove mutex if using spdlog to file */
std::mutex stdout_print_mutex;

/* not thread safe */
template <typename... Args>
void Log(FILE* out, fmt::format_string<Args...> format_str, Args&&... args) {
  // TODO: change your logger here
  fmt::print(out, format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void Log(fmt::format_string<Args...> format_str, Args&&... args) {
  // TODO: change your logger here
  std::unique_lock<std::mutex> lock(stdout_print_mutex);
  fmt::print(format_str, std::forward<Args>(args)...);
}

template <typename S, typename... Args>
void Log(const fmt::v8::text_style& ts, const S& format_str,
         const Args&... args) {
  std::unique_lock<std::mutex> lock(stdout_print_mutex);
  fmt::print(ts, format_str, args...);
}

}  // namespace lra::usb_lib
