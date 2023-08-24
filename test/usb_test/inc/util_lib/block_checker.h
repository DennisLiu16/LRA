/*
 * File: block_checker.h
 * Created Date: 2023-08-24
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday August 24th 2023 11:53:26 am
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

#include <chrono>
#include <functional>
#include <thread>

namespace lra::util {
class BlockChecker {
 public:
  BlockChecker(int block_limit_time, std::function<void()> callback);

  void start();
  void end();

 private:
  int limit;
  std::function<void()> callback_func;
  std::thread monitorThread;
  bool shouldStop;
};
}  // namespace lra::util