/*
 * File: ui_parser.hpp
 * Created Date: 2023-04-14
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday April 14th 2023 5:02:18 pm
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

#include <host_usb_lib/cdcDevice/rcws.hpp>
#include <string>

namespace lra::usb_lib {
// TODO: remove rcws, make it more general
class UIParser {
 public:
  void Parse(std::string input) {
    if (rcws_instance_ == nullptr) {
      Log("Parse error: rcws_instance_ is nullptr\n");
    } else {
      switch (input[0]) {
        case 'o': {
          auto rcws_li = rcws_instance_->FindAllRcws();
          if (rcws_li.size() == 1)
            rcws_instance_->ChooseRcws(rcws_li, 0);
          else {
            Log("Parse: {} failed. List not only one rcws existed\n", input[0]);
          }
          rcws_instance_->Open();
          break;
        }
        case 'c':
          rcws_instance_->Close();
          break;

        case 'r': {
          try {
            uint8_t index = std::stoi(input.substr(1, 1));
            if (index < LRA_DEVICE_INVALID)
              rcws_instance_->DevReset((LRA_Device_Index_t)index);
            else {
              Log("out of range");
            }
            break;
          } catch (std::exception& e) {
            Log("stoi failed\n");
            break;
          }
        }

        case 'i':
          rcws_instance_->DevInit();
          break;

        case 's': {
          try {
            uint8_t mode = std::stoi(input.substr(1, 1));
            if (mode == LRA_USB_CRTL_MODE || mode == LRA_USB_DATA_MODE) {
              rcws_instance_->DevSwitchMode((LRA_USB_Mode_t)mode);
              break;
            }
            Log("Unknown mode{}: input 2(CRTL) or 3(DATA) only\n", mode);
            break;
          } catch (std::exception& e) {
            Log("switch mode stoi failed");
            break;
          }
        }

        case 'h':
        default:
          ListCmds();
          break;
      }
    }

    Log("\nPlease input args:\n");
  }

  void RegisterRcws(Rcws* rcws) { rcws_instance_ = rcws; }

  void ListCmds() {
    Log("\nAll commands:\n");
    Log("\t(o)open\n");
    Log("\t(i)init rcws device\n");
    Log("\t(c)close \n");
    Log("\t(r)reset\n");
    Log("\t(s)switch mode\n");
    Log("\t(h)help\n");
  }

 private:
  template <typename... Args>
  void Log(fmt::format_string<Args...> format_str, Args&&... args) {
    // TODO: change your logger here
    fmt::print(format_str, std::forward<Args>(args)...);
  }

  Rcws* rcws_instance_{nullptr};
};
}  // namespace lra::usb_lib
