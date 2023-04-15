/*
 * File: non_blocking_input.hpp
 * Created Date: 2023-04-10
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Saturday April 15th 2023 9:56:15 am
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
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <host_usb_lib/parser/ui_parser.hpp>
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

  // XXX: It's for uncomplete CLI interface
  std::string PopFirst() {
    std::unique_lock<std::mutex> lock(input_queue_mutex_);
    input_queue_cv_.wait(
        lock, [this]() { return !input_queue_.empty() || exit_flag_.load(); });
    if (input_queue_.empty()) return {};
    std::string input = input_queue_.front();
    input_queue_.pop();
    return input;
  }

  bool InputAvailable() { return !input_queue_.empty(); }

  void ProcessInput() {
    while (!input_queue_.empty()) {
      std::string input = PopFirst();

      // general parse
      if (input == "exit" || input == "e" || input == "q" || input == "quit") {
        fmt::print(fg(fmt::terminal_color::red), "exit RCWS CLI\n");
        exit_flag_.store(true);
        break;
      } else {
        uiparser_.Parse(input);
      }
    }
  }

  bool GetExitFlag() { return exit_flag_.load(); }

  UIParser uiparser_;

 private:
  void InputLoop() {
    while (!exit_flag_.load()) {
      std::unique_lock<std::mutex> lock(input_queue_mutex_, std::defer_lock);
      if (std::cin.peek() != EOF) {
        lock.lock();
        std::string input;
        std::getline(std::cin, input);
        input_queue_.push(input);
        input_queue_cv_.notify_one();
        lock.unlock();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::thread input_thread_;
  std::atomic<bool> exit_flag_;
  std::mutex input_queue_mutex_;
  std::condition_variable input_queue_cv_;
  std::queue<std::string> input_queue_;
};
}  // namespace lra::usb_lib