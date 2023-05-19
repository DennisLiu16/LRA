/*
 * File: ui_parser.hpp
 * Created Date: 2023-04-14
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday May 15th 2023 11:31:24 am
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

#include <functional>
#include <host_usb_lib/cdcDevice/rcws.hpp>
#include <host_usb_lib/logger/logger.hpp>
#include <string>

namespace lra::usb_lib {
// TODO: remove rcws, make it more general
class UIParser {
 public:
  using NextInputCallback = std::function<std::string()>;

  explicit UIParser(NextInputCallback callback)
      : next_input_callback_(callback) {}

  void Parse(const std::string& input) {
    if (rcws_instance_ == nullptr) {
      Log("Parse error: rcws_instance_ is nullptr\n");
    } else {
      /* general commands parse */
      if (input == "exit") {
        rcws_instance_->Close();
        return;
      } else if (input == "clear") {
        // clear terminal
        system("clear");
      }

      /* rcws commands parse */
      else {
        switch (input[0]) {
          case 'o': {
            auto rcws_li = rcws_instance_->FindAllRcws();

            if (rcws_li.size() == 0) {
              Log(fg(fmt::terminal_color::bright_red), "No RCWS detected\n");
              break;
            } else if (rcws_li.size() == 1) {
              rcws_instance_->ChooseRcws(rcws_li, 0);
              Log(fg(fmt::terminal_color::bright_blue),
                  "Only one RCWS detected, pick that one\n");
            } else {
              /* List all RCWS */
              Log("{:5} {:^15} {:^20}\n", "Index", "Serial Number", "Path");
              for (size_t idx = 0; const auto& info : rcws_li) {
                Log("{:5} {:^15} {:^20}\n", idx++, info.serialnum, info.path);
              }

              std::string s_rcws_index = next_input_callback_();

              try {
                size_t rcws_index = std::stoi(s_rcws_index);
                if (rcws_index < rcws_li.size()) {
                  rcws_instance_->ChooseRcws(rcws_li, rcws_index);
                  Log(fg(fmt::terminal_color::bright_green), "Select Index: {}",
                      rcws_index);
                } else {
                  Log(fg(fmt::terminal_color::bright_red),
                      "RCWS index out of range\n");
                }
              } catch (std::exception& e) {
                Log("{}\n", e.what());
              }
            }

            rcws_instance_->PrintSelfInfo();
            try {
              Log(fg(fmt::terminal_color::bright_green),
                  "Try to open selected RCWS\n");
              rcws_instance_->Open();
            } catch (std::exception& e) {
              Log(fg(fmt::terminal_color::bright_red), "{}\n",
                  "Try to open RCWS failed\nPlease retry\n");
              Log(fg(fmt::terminal_color::bright_red), "{}\n", e.what());
            }

            break;
          }
          case 'c':
            rcws_instance_->Close();
            break;

          case 'r': {
            ListMap(reset_rcws_device_index_map);

            std::string s_dev_index = next_input_callback_();

            try {
              uint8_t index = std::stoi(s_dev_index);
              if (index < LRA_DEVICE_INVALID)
                rcws_instance_->DevReset((LRA_Device_Index_t)index);
              else {
                Log("out of range");
              }
            } catch (std::exception& e) {
              Log("{}\n", e.what());
            }
            break;
          }

          case 'i':
            rcws_instance_->DevInit();
            break;

          case 's': {
            ListMap(usb_mode_map);

            std::string s_mode = next_input_callback_();

            try {
              uint8_t mode = std::stoi(s_mode);
              if (mode == LRA_USB_CRTL_MODE || mode == LRA_USB_DATA_MODE) {
                rcws_instance_->DevSwitchMode((LRA_USB_Mode_t)mode);
                break;
              }
              Log(fg(fmt::terminal_color::bright_red),
                  "Unknown mode{}: input 2(CRTL) or 3(DATA) only\n", mode);
              break;
            } catch (std::exception& e) {
              Log(fg(fmt::terminal_color::bright_red),
                  "switch mode stoi failed\n");
              break;
            }
          }

          case 'p': {
          } break;

          case 'g': {
            std::vector<uint8_t> data;

            /* get device index  */
            ListMap(modify_rcws_device_index_map);

            std::string s_dev_index{""};
            while (s_dev_index.empty()) {
              s_dev_index = next_input_callback_();
            }

            try {
              uint8_t dev_index = std::stoi(s_dev_index);
              if (dev_index >= LRA_DEVICE_INVALID)
                throw std::runtime_error("device index out of range");
              data.push_back(dev_index);

            } catch (std::exception& e) {
              Log(fg(fmt::terminal_color::bright_red), "{}\n", e.what());
              break;
            }

            /* get begin iaddr */
            Log("{}:", "Please input begin iaddr");
            std::string iaddr_begin{""};

            while (iaddr_begin.empty()) {
              iaddr_begin = next_input_callback_();
            }

            try {
              uint8_t u8_iaddr_begin = std::stoi(iaddr_begin, nullptr, 0);
              Log(fg(fmt::terminal_color::red), "Begin address: 0x{:02X}\n",
                  u8_iaddr_begin);
              data.push_back(u8_iaddr_begin);

            } catch (std::exception& e) {
              Log("{}\n", e.what());
              break;
            }

            /* get begin iaddr */
            Log("{}:", "Please input begin iaddr");
            std::string iaddr_end{""};

            while (iaddr_end.empty()) {
              iaddr_end = next_input_callback_();
            }

            try {
              uint8_t u8_iaddr_end = std::stoi(iaddr_end, nullptr, 0);
              Log(fg(fmt::terminal_color::red), "End address: 0x{:02X}\n",
                  u8_iaddr_end);
              data.push_back(u8_iaddr_end);

            } catch (std::exception& e) {
              Log("{}\n", e.what());
              break;
            }

            rcws_instance_->DevSend(USB_OUT_CMD_GET_REG, data);
          }

          /* ignore */
          case 'h':
          default:
            break;
        }
      }

      ListCmds();
      fflush(stdout);
    }
  }

  void RegisterRcws(Rcws* rcws) { rcws_instance_ = rcws; }

  void ListCmds() {
    Log("\n================================================\n");
    Log("RCWS commands:\n");
    Log("\t(o)open\n");
    Log("\t(i)init rcws device\n");
    Log("\t(c)close \n");
    Log("\t(r)reset\n");
    Log("\t(s)switch mode\n");
    Log("\t(g)get register\n");

    Log("\nGeneral commands:\n");
    Log("\t(e/q)exit\n");
    Log("\t(h)help\n");
    Log("\tclear\n");
    Log(fg(fmt::terminal_color::bright_blue), "-> ");

    fflush(stdout);
  }

  template <typename Key, typename Value>
  void ListMap(const std::map<Key, Value>& map) {
    for (const auto& [key, value] : map) {
      Log("{}: {}\n", key, value);
    }
  }

 private:
  // init in init list
  NextInputCallback next_input_callback_;

  Rcws* rcws_instance_{nullptr};
};
}  // namespace lra::usb_lib
