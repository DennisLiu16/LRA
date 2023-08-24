/*
 * File: ui_parser.hpp
 * Created Date: 2023-04-14
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday August 24th 2023 12:01:36 pm
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

#include <fcntl.h>
#include <host_usb_lib/cdcDevice/rcws.h>
#include <host_usb_lib/logger/logger.h>
#include <host_usb_lib/realtime/realtime_plot.h>
#include <unistd.h>
#include <util_lib/block_checker.h>

#include <functional>
#include <string>
#include <util_lib/range_bound.hpp>

namespace lra::usb_lib {
// TODO: remove rcws, make it more general

/* TODO: a static function, remove to util */
static int set_close_on_exec(int fd) {
  int flags = fcntl(fd, F_GETFD);
  if (flags == -1) return -1;
  flags |= FD_CLOEXEC;
  return fcntl(fd, F_SETFD, flags);
}

class PathDoesNotExistException : public std::runtime_error {
 public:
  PathDoesNotExistException(const std::filesystem::path& p)
      : std::runtime_error("Path does not exist: " + p.string()) {}
};

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
                  "Exception: Try to open RCWS failed\nPlease retry\n");
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
                  rcws_instance_->UpdateAccFileName(next_acc_full_path);
                }

                if (pwm_tmp != nullptr) {
                  rcws_instance_->UpdatePwmFileHandle(pwm_tmp);
                  Log(fg(fmt::terminal_color::bright_blue),
                      "open pwm file:{}\n", next_pwm_full_path);
                  rcws_instance_->UpdatePwmFileName(next_pwm_full_path);
                }

                /* create pipe line for python real time plot */
                do {
                  if (!realtime_plot::isRunningInWSL()) {
                    break;
                  }

                  /* get correct pipe path */
                  RcwsInfo target_rcws_info = rcws_instance_->GetRcwsInfo();

                  if (target_rcws_info.path.empty()) {
                    Log(fg(fmt::terminal_color::bright_red),
                        "target_rcws_info.path is empty. Fail to get pipe "
                        "name"
                        "for python real time plot\n");
                    break;
                  }

                  std::string pipe_root_path =
                      rcws_instance_->data_path_ + "/" + "pipe";

                  /* check path exists or create */
                  lra::realtime_plot::ensureDirectoryExists(pipe_root_path);

                  std::string pipe_path =
                      pipe_root_path + "/" + target_rcws_info.serialnum;

                  bool create_pipe_state = realtime_plot::createPipe(pipe_path);

                  if (create_pipe_state == false) {
                    Log(fg(fmt::terminal_color::bright_red),
                        "Fail to create given pipe name: {}\n", pipe_path);
                    break;
                  }

                  /* create pipe successfully */
                  Log(fg(fmt::terminal_color::bright_green),
                      "Pipe created: {}\n", pipe_path);

                  /* start python program */
                  /* TODO: change this path */
                  std::string python_program_path =
                      "/home/dennis/develop/LRA/test/usb_test/script/"
                      "realtime_plot.py";

                  /* XXX: debug log */
                  std::string log_python_root_path =
                      rcws_instance_->data_path_ + "/" + "pipe_log";

                  lra::realtime_plot::ensureDirectoryExists(
                      log_python_root_path);

                  std::string log_python_path = log_python_root_path + "/" +
                                                target_rcws_info.serialnum +
                                                ".txt";

                  std::string exe_python = "python3 " + python_program_path +
                                           " " + pipe_path + " >" +
                                           log_python_path + " 2>&1 &";

                  /* Error: this cause libserial fd inherit to child
                   * process. Therefore, next serial open will fail due to error
                   * 'Device or resource busy' */

                  // std::system(exe_python.c_str());

                  /* system going to fork */
                  Log(fg(fmt::terminal_color::bright_blue),
                      "Going to fork current process\n");

                  /* fork */
                  pid_t pid = fork();

                  if (pid == 0) {
                    /* child process */
                    close(STDIN_FILENO);
                    int original_stdout = dup(STDOUT_FILENO);

                    freopen(log_python_path.c_str(), "w", stdout);

                    /* TODO: add conda activate base to exe_python */

                    /* close libserial file descriptor after exec func */
                    int serial_fd = rcws_instance_->GetLibSerialFd();
                    int result = set_close_on_exec(serial_fd);

                    if (result < 0) {
                      Log(fg(fmt::terminal_color::bright_red),
                          "Set libserial fd close on exec failed!\n");
                      break;
                    }

                    /* exe here, never return if exec ok */
                    execl("/bin/bash", "bash", "-c", exe_python.c_str(),
                          (char*)NULL);

                    /* if fail */
                    dup2(original_stdout, STDOUT_FILENO);
                    close(original_stdout);
                    Log(fg(fmt::terminal_color::bright_red),
                        "Execl python realtime plot script failed\n");
                    _exit(0);
                  } else if (pid < 0) {
                    Log(fg(fmt::terminal_color::bright_red),
                        "PID < 0, unknown error.\n");
                    break;
                  } /* else is parent pid */

                  /* TODO: check execl state */

                  /* create block checker */
                  auto checker = lra::util::BlockChecker(5, [] {
                    Log(fg(fmt::terminal_color::bright_red),
                        "Please check current terminal has python env that "
                        "conform to realtime_plot.py requiring\n");
                  });

                  checker.start();

                  /* open and transmit files name */
                  int pipe_fd = open(pipe_path.c_str(), O_WRONLY);
                  std::string msg =
                      next_pwm_full_path + "," + next_acc_full_path;
                  write(pipe_fd, msg.c_str(), msg.length());
                  close(pipe_fd);

                  checker.end();

                  Log(fg(fmt::terminal_color::bright_green),
                      "Send files information to pipe line.\n");

                  /* assign to rcws instance */
                  rcws_instance_->pipe_name_ = pipe_path;

                } while (0);
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

              if (mode == LRA_USB_CRTL_MODE &&
                  (!rcws_instance_->pipe_name_.empty())) {
                /* send eof to python end */
                int pipe_fd =
                    open(rcws_instance_->pipe_name_.c_str(), O_WRONLY);
                std::string stop_signal = "eof";
                write(pipe_fd, stop_signal.c_str(), stop_signal.length());
                close(pipe_fd);

                rcws_instance_->pipe_name_ = "";

                Log(fg(fmt::terminal_color::bright_green),
                    "Send eof to pipe line. Python should be closed.\n");
              }

              break;
            } catch (const std::invalid_argument& e) {
              Log(fg(fmt::terminal_color::bright_red),
                  "switch mode stoi failed\n");
              break;
            }
          }

          /* add auto generate function */
          case 'p': {
            ListMap(pwm_cmd_mode_map);

            std::string s_mode = next_input_callback_();

            try {
              uint8_t mode = std::stoi(s_mode);
              if (mode != RCWS_PWM_MANUAL_MODE && mode != RCWS_PWM_FILE_MODE) {
                Log(fg(fmt::terminal_color::bright_red), "Invalid mode: {} \n",
                    mode);
                break;
              }

              if (mode == RCWS_PWM_MANUAL_MODE) {
                /* 6 floats POD */
                RcwsPwmInfo info;
                constexpr size_t float_num =
                    sizeof(RcwsPwmInfo) / sizeof(float);
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
                  std::string hint =
                      Format("{}-{}\n", (char)(axis + axis_offset),
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
              } else {  // RCWS_PWM_FILE_MODE

                /* TODO: check is running */
                if (rcws_instance_->PwmCmdThreadRunning()) {
                  Log("Pwm cmd thread is already running, if you want to "
                      "cancel it, please input 'y' or 'yes'\n");
                  std::string s_cancel = next_input_callback_();

                  /* cancel, clean the queue */
                  if (s_cancel == "y" || s_cancel == "yes") {
                    rcws_instance_->PwmCmdThreadClose();
                    rcws_instance_->CleanPwmCmdQueue();
                  } else
                    break;

                } else {  // load new pwm cmd csv
                  std::filesystem::path path =
                      rcws_instance_->data_path_ + "/pwm_cmd_file";

                  if (!std::filesystem::exists(path)) {
                    throw PathDoesNotExistException(path);
                  }

                  /* list all csv */
                  int file_index = 0;
                  std::map<int, std::string> csvFiles;

                  for (const auto& entry :
                       std::filesystem::directory_iterator(path)) {
                    if (entry.path().extension() == ".csv") {
                      csvFiles[file_index++] = entry.path().string();
                    }
                  }

                  if (csvFiles.size() == 0) {
                    Log(fg(fmt::terminal_color::bright_red),
                        "There's no any file in given directory: {}\n", path);
                    break;
                  }

                  ListMap(csvFiles);

                  /* user choose csv */
                  std::string hint = "Choose the file you want to import\n";
                  int csv_index = GetInt(hint);
                  if (!csvFiles.count(csv_index)) {
                    Log("Invalid index: {}\n", csv_index);
                    break;
                  }

                  std::string csv_file_path = csvFiles[csv_index];

                  /* recursive or not */
                  Log("recursive simulate given csv\n. Input 'y' or 'yes' if "
                      "you want to simulate in forever loop\n");
                  std::string recursive_y = next_input_callback_();

                  if (recursive_y == "y" || recursive_y == "yes") {
                    rcws_instance_->PwmCmdSetRecursive(true);
                  } else {
                    rcws_instance_->PwmCmdSetRecursive(false);
                  }

                  // create task
                  rcws_instance_->StartPwmCmdThread(csv_file_path);
                }
              }
            } catch (const PathDoesNotExistException& e) {
              Log(fg(fmt::terminal_color::bright_red), "path not exist: {}\n",
                  e.what());
            } catch (std::exception& e) {
              Log(fg(fmt::terminal_color::bright_red), "stoi failed\n");
              break;
            }
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

  int GetInt(const std::string& output) {
    Log("{}", output);

    std::string s_float = next_input_callback_();
    return std::stof(s_float);
  }
};
}  // namespace lra::usb_lib
