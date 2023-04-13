/*
 * File: usb_test_v1.1.cc
 * Created Date: 2023-03-30
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday April 11th 2023 10:45:59 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <spdlog/fmt/fmt.h>

#include <host_usb_lib/cdcDevice/rcws.hpp>
#include <host_usb_lib/userInput/non_blocking_input.hpp>

using namespace lra::usb_lib;

int main() {
  // add nonblocking user input
  NonBlockingInput non_blocking_input;

  auto rcws_instance = lra::usb_lib::Rcws();

  auto rcws_list = rcws_instance.FindAllRcws();

  rcws_instance.PrintAllRcwsInfo(rcws_list);

  // add RCWS here

  // register parser
  // non_blocking_input.RegisterParser(
  //     [](const std::string& input) { std::cout << "You entered: " << input <<
  //     std::endl; });

  while (non_blocking_input.GetExitFlag()) {
    non_blocking_input.ProcessInput();

    // take a reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
