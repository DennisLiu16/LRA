/*
 * File: ui_parser.hpp
 * Created Date: 2023-04-14
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:33:32 pm
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

#include <host_usb_lib/cdcDevice/rcws.h>
#include <host_usb_lib/logger/logger.h>

#include <functional>
#include <string>
#include <util/range_bound.hpp>

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
              if (mode != LRA_USB_CRTL_MODE && mode != LRA_USB_DATA_MODE) {
                Log(fg(fmt::terminal_color::bright_red),
                    "Unknown mode{}: input 2(CRTL) or 3(DATA) only\n", mode);
                break;
              }

              if (mode == LRA_USB_DATA_MODE) {
                std::string next_acc_file_name =
                    rcws_instance_->GetNextFileName(rcws_instance_->data_path_,
                                                    "acc");
                std::string next_pwm_file_name =
                    rcws_instance_->GetNextFileName(rcws_instance_->data_path_,
                                                    "pwm");

                std::string next_acc_full_path =
                    rcws_instance_->data_path_ + "/" + next_acc_file_name;
                std::string next_pwm_full_path =
                    rcws_instance_->data_path_ + "/" + next_pwm_file_name;

                FILE* acc_tmp = fopen(next_acc_full_path.c_str(), "a");
                FILE* pwm_tmp = fopen(next_pwm_full_path.c_str(), "a");

                if (acc_tmp != nullptr) {
                  rcws_instance_->UpdateAccFileHandle(acc_tmp);
                  Log(fg(fmt::terminal_color::bright_blue),
                      "open acc file:{}\n", next_acc_full_path);
                  rcws_instance_->UpdateAccFileName(next_acc_file_name);
                }

                if (pwm_tmp != nullptr) {
                  rcws_instance_->UpdatePwmFileHandle(pwm_tmp);
                  Log(fg(fmt::terminal_color::bright_blue),
                      "open pwm file:{}\n", next_pwm_full_path);
                  rcws_instance_->UpdatePwmFileName(next_pwm_full_path);
                }

              } else {
                FILE* acc_file = rcws_instance_->GetAccFileHandle();
                FILE* pwm_file = rcws_instance_->GetPwmFileHandle();

                if (acc_file != nullptr) {
                  fclose(acc_file);
                  Log(fg(fmt::terminal_color::bright_blue),
                      "close acc file:{}\n", rcws_instance_->GetAccFileName());
                  rcws_instance_->UpdateAccFileName("");
                  rcws_instance_->UpdateAccFileHandle(nullptr);
                }

                if (pwm_file != nullptr) {
                  fclose(pwm_file);
                  Log(fg(fmt::terminal_color::bright_blue),
                      "close pwm file:{}\n", rcws_instance_->GetPwmFileName());
                  rcws_instance_->UpdatePwmFileName("");
                  rcws_instance_->UpdatePwmFileHandle(nullptr);
                }
              }

              rcws_instance_->DevSwitchMode((LRA_USB_Mode_t)mode);

              break;
            } catch (std::exception& e) {
              Log(fg(fmt::terminal_color::bright_red),
                  "switch mode stoi failed\n");
              break;
            }
          }

          /* add auto generate function */
          case 'p': {
            /* 6 floats POD */
            RcwsPwmInfo info;
            constexpr size_t float_num = sizeof(RcwsPwmInfo) / sizeof(float);
            float fdata[float_num];

            char axis = 'x';
            std::string s_amp = "amp: from 0 < amp < 1000";
            std::string s_freq = "freq: from 1.0 < freq < 10.0";

            lra::util::Range amp_range(0, 1000);
            lra::util::Range freq_range(1.0, 10.0);

            /* get 6 floats */
            int valid_float = 0;
            while (valid_float < float_num) {
              char axis_offset = valid_float / 2;
              std::string hint = Format("{}-{}\n", (char)(axis + axis_offset),
                                        (valid_float % 2) ? s_freq : s_amp);
              try {
                float float_candidate = GetFloat(hint);
                bool float_is_ok = false;

                if (valid_float % 2) {
                  float_is_ok = freq_range.isWithinRange(float_candidate);
                } else {
                  float_is_ok = amp_range.isWithinRange(float_candidate);
                }

                if (float_is_ok) {
                  fdata[valid_float] = float_candidate;
                  valid_float++;
                  continue;
                }

                Log("Invalid Input\n");

              } catch (std::exception& e) {
                Log("{}: Please check your input!\n", e.what());
              }
            }

            // assign to RcwsPwmInfo
            memcpy(&info, fdata, sizeof(RcwsPwmInfo));

            // TODO: create PrintRcwsPwmInfo
            rcws_instance_->PrintRcwsPwmInfo(info);

            // update to RCWS
            rcws_instance_->DevPwmCmd(info);

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
    Log("\t(p)pwm cmd\n");

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

  /*  private functions */
  float GetFloat(const std::string& output) {
    Log("{}", output);

    std::string s_float = next_input_callback_();
    return std::stof(s_float);
  }
};
}  // namespace lra::usb_lib
