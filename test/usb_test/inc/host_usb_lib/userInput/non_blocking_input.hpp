/*
 * File: non_blocking_input.hpp
 * Created Date: 2023-04-10
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday April 11th 2023 3:35:02 pm
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

#include <spdlog/fmt/fmt.h>

#include <atomic>
#include <functional>
#include <host_usb_lib/parser/parser.hpp>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace lra::usb_lib {
class NonBlockingInput {
 public:
  NonBlockingInput() : exit_flag_(false) {
    input_thread_ = std::thread(&NonBlockingInput::InputLoop, this);
  }

  ~NonBlockingInput() {
    exit_flag_.store(true);
    input_thread_.join();
  }

  void ProcessInput() {
    std::unique_lock<std::mutex> lock(input_queue_mutex_);
    while (!input_queue_.empty()) {
      std::string input = input_queue_.front();
      input_queue_.pop();

      // exit
      if (input == "exit" || input == "e" || input == "q" || input == "quit") {
        fmt::print("exit RCWS CLI\n");
        exit_flag_.store(true);
      }
      uiparser_.Parse(input);
    }
  }

  bool GetExitFlag() { return exit_flag_.load(); }

 private:
  void InputLoop() {
    while (!exit_flag_.load()) {
      std::string input;
      std::getline(std::cin, input);

      std::unique_lock<std::mutex> lock(input_queue_mutex_);
      input_queue_.push(input);
    }
  }

  std::thread input_thread_;
  std::atomic<bool> exit_flag_;
  std::mutex input_queue_mutex_;
  std::queue<std::string> input_queue_;
  UIParser uiparser_;
};
}  // namespace lra::usb_lib