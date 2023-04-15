/*
 * File: usb_test_v1.1.cc
 * Created Date: 2023-03-30
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Saturday April 15th 2023 10:54:38 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <fft_lib/file_loader/csv.h>
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/fmt.h>

#include <filesystem>
#include <host_usb_lib/cdcDevice/rcws.hpp>
#include <host_usb_lib/userInput/non_blocking_input.hpp>
#include <string>

using namespace lra::usb_lib;

int main(int argc, char* argv[]) {
  Log("\n");

  // add nonblocking user input
  NonBlockingInput non_blocking_input;

  auto rcws_instance = lra::usb_lib::Rcws();

  // auto rcws_list = rcws_instance.FindAllRcws();

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

  /* read csv data */
  try {
    std::filesystem::path current_bin_path =
        std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();

    std::filesystem::path root_path =
        current_bin_path.parent_path().parent_path();
    std::filesystem::path csv_path =
        root_path / "test/usb_test/data/f10000.csv";

    Log(fg(fmt::terminal_color::bright_blue), "Target csv path: {}\n",
        csv_path.string());

    io::CSVReader<4> csv_in(csv_path.string());

    /* ignore first n lines, n = 18 */
    constexpr int skip_lines = 18;
    for (int i = 0; i < skip_lines; i++) {
      csv_in.next_line();
    }

    csv_in.read_header(io::ignore_extra_column, "Time", "Fx", "Fy", "Fz");

    /* pass unit */
    std::string u1, u2, u3, u4;
    csv_in.read_row(u1, u2, u3, u4);

    /* read data */
    std::vector<float> data_t, data_x, data_y, data_z;
    float t, x, y, z;

    while (csv_in.read_row(t, x, y, z)) {
      data_t.push_back(t);
      data_x.push_back(x);
      data_y.push_back(y);
      data_z.push_back(z);
    }

    Log(fg(fmt::terminal_color::bright_blue),
        "Reading CSV completed, total rows:{}\n", data_t.size());
  } catch (std::exception& e) {
    Log("{}\n", e.what());
  }

  non_blocking_input.uiparser_.RegisterRcws(&rcws_instance);
  non_blocking_input.uiparser_.ListCmds();

  while (!non_blocking_input.GetExitFlag()) {
    non_blocking_input.ProcessInput();

    // take a reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
