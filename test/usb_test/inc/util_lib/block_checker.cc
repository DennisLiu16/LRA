/*
 * File: block_checker.cc
 * Created Date: 2023-08-24
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday August 24th 2023 11:53:21 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include "block_checker.h"

namespace lra::util {
BlockChecker::BlockChecker(int block_limit_time, std::function<void()> callback)
    : limit(block_limit_time), callback_func(callback), shouldStop(false) {}

void BlockChecker::start() {
  monitorThread = std::thread([this] {
    std::this_thread::sleep_for(std::chrono::seconds(this->limit));
    if (!this->shouldStop) {
      this->callback_func();
    }
  });
}

void BlockChecker::end() {
  shouldStop = true;
  if (monitorThread.joinable()) {
    monitorThread.join();
  }
}

}  // namespace lra::util
