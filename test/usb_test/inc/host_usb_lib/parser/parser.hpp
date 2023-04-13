/*
 * File: parser.hpp
 * Created Date: 2023-03-31
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday April 11th 2023 6:53:20 pm
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
#include <spdlog/fmt/fmt.h>

#include <host_usb_lib/command/command.hpp>
#include <string>
#include <variant>
#include <vector>

namespace lra::usb_lib {
class RcwsParser {
 public:
  RcwsCmdType Parse(const std::string& data) const {}

  void RegisterCmd(std::vector<RcwsCmdType> commands) {
    available_cmd_.insert(available_cmd_.end(), commands.begin(),
                          commands.end());
  }

  std::vector<RcwsCmdType> filterByMode(uint8_t mode) {
    std::vector<RcwsCmdType> ret;

    for (const auto& cmd : available_cmd_) {
      std::visit(
          [&ret, mode](auto&& cmd_variant) {
            using T = std::decay_t<decltype(cmd_variant)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
              uint8_t required_mode = cmd_variant.get_mode();
              if (required_mode == mode ||
                  required_mode == LRA_USB_ALL_AVAILABLE_MODE) {
                ret.push_back(cmd_variant);
              }
            }
          },
          cmd);
    }
    return ret;
  }

  void PrintAvailableCmd(uint8_t mode) {
    auto cmd_candidate = filterByMode(mode);
    fmt::print("Available Cmd in mode: {}\n", mode);
    /* print */
    int index = 0;
    for (const auto& cmd : cmd_candidate) {
      std::visit(
          [index](auto&& cmd_variant) {
            using T = std::decay_t<decltype(cmd_variant)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
              fmt::print("{}\t{}\t{}\n", index, cmd_variant.get_name(),
                         cmd_variant.get_description());
            }
          },
          cmd);
      index++;
    }
    fmt::print("\n\nPlease input index:\n");
  }

 protected:
  std::vector<RcwsCmdType> available_cmd_;
};

class UIParser {
 public:
  UiCmdType Parse(std::string input) {}
};
}  // namespace lra::usb_lib