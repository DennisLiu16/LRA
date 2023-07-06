// /*
//  * File: parser.hpp
//  * Created Date: 2023-03-31
//  * Author: Dennis Liu
//  * Contact: <liusx880630@gmail.com>
//  *
//  * Last Modified: Thursday July 6th 2023 4:07:48 pm
//  *
//  * Copyright (c) 2023 None
//  *
//  * -----
//  * HISTORY:
//  * Date      	 By	Comments
//  * ----------	---
//  * ----------------------------------------------------------
//  */

// #pragma once

// #include <drv_stm_lib/lra_usb_defines.h>
// #include <host_usb_lib/cdcDevice/rcws.hpp>
// #include <host_usb_lib/command/command.hpp>
// #include <host_usb_lib/logger/logger.h>
// #include <map>
// #include <string>
// #include <variant>
// #include <vector>

// namespace lra::usb_lib {
// /* variant version */

// // class RcwsParser {
// //    public:
// //     RcwsCmdType Parse(const std::string& data) const {}

// //     void RegisterCmd(std::vector<RcwsCmdType> commands) {
// //       available_cmd_.insert(available_cmd_.end(), commands.begin(),
// //                             commands.end());
// //     }

// //     std::vector<RcwsCmdType> filterByMode(uint8_t mode) {
// //       std::vector<RcwsCmdType> ret;

// //       for (const auto& cmd : available_cmd_) {
// //         std::visit(
// //             [&ret, mode](auto&& cmd_variant) {
// //               using T = std::decay_t<decltype(cmd_variant)>;
// //               if constexpr (!std::is_same_v<T, std::monostate>) {
// //                 uint8_t required_mode = cmd_variant.get_mode();
// //                 if (required_mode == mode ||
// //                     required_mode == LRA_USB_ALL_AVAILABLE_MODE) {
// //                   ret.push_back(cmd_variant);
// //                 }
// //               }
// //             },
// //             cmd);
// //       }
// //       return ret;
// //     }

// //     void PrintAvailableCmd(uint8_t mode) {
// //       auto cmd_candidate = filterByMode(mode);
// //       fmt::print("Available Cmd in mode: {}\n", mode);
// //       /* print */
// //       int index = 0;
// //       for (const auto& cmd : cmd_candidate) {
// //         std::visit(
// //             [index](auto&& cmd_variant) {
// //               using T = std::decay_t<decltype(cmd_variant)>;
// //               if constexpr (!std::is_same_v<T, std::monostate>) {
// //                 fmt::print("{}\t{}\t{}\n", index, cmd_variant.get_name(),
// //                            cmd_variant.get_description());
// //               }
// //             },
// //             cmd);
// //         index++;
// //       }
// //       fmt::print("\n\nPlease input index:\n");
// //     }

// //    protected:
// //     std::vector<RcwsCmdType> available_cmd_;
// // };

// // XXX: forward declaration of Rcws
// class Rcws;

// /* traditional version */
// class RcwsParser {
//  public:
//   void Parse(const std::vector<uint8_t>& msg) {
//     std::string log_msg;

//     switch (msg[0]) {
//       case USB_IN_CMD_PARSE_ERR: {
//         uint16_t err_info = msg[3] << 8 | msg[4];

//         std::string tmp_err_type_msg{}, tmp_cmd_msg{};

//         // get error msg
//         for (const auto& [err_code, err_type_msg] : rcws_error_state_map) {
//           if (err_code & err_info) {
//             tmp_err_type_msg = err_type_msg;
//             break;
//           }
//         }

//         // get cmd type msg
//         for (const auto& [cmd_type, cmd_type_msg] : usb_basic_cmd_type_map) {
//           if (cmd_type == (uint8_t)(err_info & BASIC_CMD_MASK)) {
//             tmp_cmd_msg = cmd_type_msg;
//             break;
//           }
//         }

//         log_msg = fmt::format(fg(fmt::terminal_color::bright_red),
//                               "\nCmd type: {}\n"
//                               "Error type: {}\n",
//                               tmp_cmd_msg, tmp_err_type_msg);
//         break;
//       }

//       case USB_IN_CMD_INIT: {
//         std::string rx_init_string = std::string(msg.begin() + 3, msg.end());
//         if (rx_init_string != rcws_msg_init)
//           log_msg = "Init string incorrect";
//         else
//           log_msg = "Init string correct, start to init";
//         break;
//       }

//       case USB_IN_CMD_RESET_DEVICE:
//         log_msg = fmt::format(
//             "Successfully reset device: {}",
//             reset_rcws_device_index_map[(LRA_Device_Index_t)msg[3]]);
//         break;

//       case USB_IN_CMD_UPDATE_PWM: {
//         uint16_t data_len = msg[1] << 8 | msg[2];

//         const uint8_t* pos = &msg[3];

//         RcwsPwmInfo info;
//         float t;

//         ParsePwmInData(pos, &info, &t);

//         std::string to_file = fmt::format("{:.3f}, {}, {}, {}, {}, {}, {}\n",
//         t,
//                                           info.x.amp, info.x.freq,
//                                           info.y.amp, info.y.freq,
//                                           info.z.amp, info.z.freq);

//         if (prcws_ && prcws_->GetPwmFileHandle()) {
//           Log(prcws_->GetPwmFileHandle(), to_file);
//         }

//         break;
//       }

//       case USB_IN_CMD_UPDATE_ACC: {
//         uint16_t data_len = msg[1] << 8 | msg[2];

//         // verify length
//         if ((data_len - 2) % sizeof(ADXL355_DataSet_t) != 0) {
//           // length mismatch
//           log_msg = fmt::format("Acc data len mismatch:{}");
//           break;
//         }

//         uint16_t data_set_len = (data_len - 2) / sizeof(ADXL355_DataSet_t);
//         std::string acc_log_content;
//         // 30 is approximately number of one line of log data
//         acc_log_content.reserve(data_set_len * 30);
//         auto out = std::back_inserter(acc_log_content);

//         ADXL355_DataSet_t acc_data_set;
//         const uint8_t* ptr = &msg[3];

//         for (int i = 0; i < data_set_len; i++) {
//           memcpy(&acc_data_set, ptr + i * sizeof(ADXL355_DataSet_t),
//                  sizeof(ADXL355_DataSet_t));
//           fmt::format_to(out, "{:.6f}, {:.4f}, {:.4f}, {:.4f}\n",
//                          acc_data_set.t, acc_data_set.t,
//                          acc_data_set.data[0], acc_data_set.data[1],
//                          acc_data_set.data[2]);
//         }

//         if (prcws_ && prcws_->GetAccFileHandle()) {
//           Log(prcws_->GetAccFileHandle(), acc_log_content);
//         }

//         break;
//       }

//       case USB_IN_CMD_SWITCH_MODE:
//         log_msg = fmt::format("{}", usb_mode_map[(LRA_USB_Mode_t)msg[3]]);
//         break;

//       case USB_IN_CMD_SYS_INFO:
//         log_msg = std::string(msg.begin() + 3, msg.end());
//         break;

//       case USB_IN_CMD_GET_REG: {
//         uint16_t data_len = msg[1] << 8 | msg[2];

//         auto header = fmt::format(
//             "\n"
//             "Read from device: {}\n"
//             "Start address: 0x{:02X}\n"
//             "End address: 0x{:02X}\n"
//             "Total length: {}\n",
//             modify_rcws_device_index_map[(LRA_Device_Index_t)msg[3]], msg[4],
//             msg[5], msg[5] - msg[4] + 1);

//         int index = msg[4];
//         std::string content = fmt::format("\n{:^11} | {:^10} | {:^10}\n\n",
//                                           "Iaddr", "Hex", "Binary");

//         // msg_header (3 bytes) + get_reg header (3 bytes too)
//         auto reg_begin = msg.begin() + 6;
//         // 2 -> \r\n
//         auto reg_end = msg.end() - 2;

//         for (auto it = reg_begin; it != reg_end; ++it, ++index) {
//           auto format_binary =
//               fmt::format("{:04b} {:04b}", *it >> 4, (*it) & 0b1111);
//           auto format_hex = fmt::format("0x{:02X}", *it);
//           auto format_index = fmt::format("0x{:02X}", index);
//           content += fmt::format("{:^11} | {:^10} | {:^10}\n", format_index,
//                                  format_hex, format_binary);
//         }

//         log_msg = fmt::format("{}{}", header, content);
//         break;
//       }

//       /* never get here */
//       default:
//         log_msg = "You should never get here";
//         break;
//     }

//     /* Log info */
//     if (log_msg.empty()) {
//       return;
//     }

//     Log("\r");

//     Log(fg(fmt::terminal_color::bright_yellow), "[{}]: {}\n",
//         usb_in_cmd_type_map[(LRA_USB_IN_Cmd_t)msg[0]], log_msg);

//     /* 補輸出使用者輸入 */
//     Log(fg(fmt::terminal_color::bright_blue), "-> ");
//     fflush(stdout);
//   }

//   void RegisterDevice(Rcws* prcws) {
//     if (prcws != nullptr) prcws_ = prcws;
//   }

//   void ParsePwmInData(const uint8_t* pdata, RcwsPwmInfo* info,
//                       float* sys_time) {
//     static const size_t parameters_per_axis = 2;
//     static const size_t byte_count_per_parameter = sizeof(float);
//     static const size_t byte_count_delimiter = 1;
//     static const size_t byte_count_per_axis =
//         parameters_per_axis * (byte_count_per_parameter +
//         byte_count_delimiter);
//     static const size_t offset_freq =
//         byte_count_per_parameter + byte_count_delimiter;

//     // Get time
//     memcpy(sys_time, pdata, sizeof(float));

//     /* parse */
//     const uint8_t* pPwm = pdata + sizeof(float);
//     PwmInfo* pwm_info_arr[3] = {&(info->x), &(info->y), &(info->z)};

//     /* Assuming little-endian system, and float is 4 bytes */
//     for (size_t i = 0; i < 3; ++i) {
//       const uint8_t* amp_bytes = pPwm + i * byte_count_per_axis;
//       const uint8_t* freq_bytes = amp_bytes + offset_freq;
//       PwmInfo* pwm_info = pwm_info_arr[i];

//       /* Assuming that the platform uses IEEE 754 floating-point
//       representation
//        */
//       memcpy(&(pwm_info->amp), amp_bytes, sizeof(float));
//       memcpy(&(pwm_info->freq), freq_bytes, sizeof(float));
//     }
//   }

//  private:
//   Rcws* prcws_{nullptr};
// };

// }  // namespace lra::usb_lib