/*
 * File: command_impl.hpp
 * Created Date: 2023-04-02
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday April 14th 2023 2:32:02 pm
 *
 * Copyright (c) 2023 None
 *
 * Dependencies: boost
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#pragma once
#include <spdlog/fmt/fmt.h>

#include <functional>
#include <string>
#include <tuple>

#include "function_info.hpp"

namespace lra::usb_lib {

/**
 * How to use it: https://godbolt.org/z/GTzYcTvPn
 * use `std::bind_front` instead of std::bind
 */
template <typename FuncInfo>
class Command {
  using func_info_type = FuncInfo;

 public:
  explicit Command(const std::string& name, const std::string& description,
                   uint8_t required_mode, FuncInfo&& func_info)
      : name_(name),
        description_(description),
        mode_(required_mode),
        func_info_(std::move(func_info)) {}

  explicit Command(const std::string& name, const std::string& description,
                   FuncInfo&& func_info)
      : name_(name),
        description_(description),
        func_info_(std::move(func_info)) {}

  auto Execute() { return func_info_.call(); }

  template <typename... Args>
  auto Execute(Args... args) {
    return func_info_.call(args...);
  }

  const std::string& get_name() const { return name_; }

  const std::string& get_description() const { return description_; }

  const uint8_t get_mode() const { return mode_; }

 private:
  uint8_t mode_;
  std::string name_;
  std::string description_;
  FuncInfo func_info_;
};

// TODO: need to impl FuncInfo& version?
/* Clicommand example */
// https://godbolt.org/z/K76rPG15E

}  // namespace lra::usb_lib
