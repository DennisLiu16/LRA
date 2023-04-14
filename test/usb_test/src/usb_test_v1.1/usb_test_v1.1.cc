/*
 * File: usb_test_v1.1.cc
 * Created Date: 2023-03-30
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday April 14th 2023 4:58:47 pm
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
#include <string>

using namespace lra::usb_lib;

int main() {
  // add nonblocking user input
  NonBlockingInput non_blocking_input;

  auto rcws_instance = lra::usb_lib::Rcws();

  auto rcws_list = rcws_instance.FindAllRcws();

  // auto validate_rcws_usr_choice = [&non_blocking_input, &rcws_instance,
  //                                  &rcws_list]() {
  //   rcws_instance.PrintAllRcwsInfo(rcws_list);
  //   while (!non_blocking_input.InputAvailable()) {
  //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //   }

  //   std::string usr_input = non_blocking_input.PopFirst();
  //   try {
  //     int ret = std::stoi(usr_input);
  //     if (ret >= rcws_list.size()) throw std::runtime_error("out of range");
  //     return ret;
  //   } catch (std::exception& e) {
  //     fmt::print("Throw: {}\n", e.what());
  //     fmt::print("Please check your input\n");
  //     return -1;
  //   }
  // };

  // int final_rcws_index = -1;
  // while (final_rcws_index == -1) {
  //   final_rcws_index = validate_rcws_usr_choice();
  // }

  // rcws_instance.ChooseRcws(rcws_list, final_rcws_index);
  // rcws_instance.Open();
  // rcws_instance.DevInit();

  // TODO: 未來把 CLI 介面做完

  non_blocking_input.uiparser_.RegisterRcws(&rcws_instance);
  non_blocking_input.uiparser_.ListCmds();

  while (!non_blocking_input.GetExitFlag()) {
    non_blocking_input.ProcessInput();

    // take a reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
