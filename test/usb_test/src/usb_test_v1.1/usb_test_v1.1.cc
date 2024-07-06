/*
 * File: usb_test_v1.1.cc
 * Created Date: 2023-03-30
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Friday July 14th 2023 10:50:50 am
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <fft_lib/third_party/csv.h>
#include <host_usb_lib/cdcDevice/rcws.h>
#include <host_usb_lib/logger/logger.h>

#include <fft_lib/fft_wrapper/fft_helper.hpp>
#include <filesystem>
#include <host_usb_lib/userInput/non_blocking_input.hpp>
#include <string>

using namespace lra::fft_lib;
using namespace lra::usb_lib;

int main(int argc, char* argv[]) {
  Log("\n");

  // add nonblocking user input
  NonBlockingInput non_blocking_input;

#ifndef RCWS_LRA_DATA_PATH
#error "RCWS_LRA_DATA_PATH is not defined"
#endif

  std::string data_path = std::string(RCWS_LRA_DATA_PATH);

  Log(fg(fmt::terminal_color::bright_blue), "RCWS data path is: {}\n",
      data_path);

  auto rcws_instance = lra::usb_lib::Rcws();
  rcws_instance.data_path_ = data_path;  // defined in top CMakeLists.txt

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
  //     if (ret >= rcws_list.size()) throw std::runtime_error("out of
  //     range"); return ret;
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

    std::filesystem::path root_path = RCWS_LRA_ROOT_PATH;
    std::filesystem::path data_dir = root_path / "test/usb_test/data";

    std::filesystem::path csv_path = data_dir / "f10000.csv";

    std::filesystem::path fft_log_path = data_dir / "log/fft.txt";

    Log(fg(fmt::terminal_color::bright_blue), "Target csv path: {}\n",
        csv_path.string());

    io::CSVReader<4> csv_in(csv_path.string());

    /* ignore first n lines, n = 18 */
    constexpr int skip2sampling_rate = 10;
    for (int i = 0; i < skip2sampling_rate; i++) {
      csv_in.next_line();
    }

    /* get sampling rate */
    std::string sampling_rate_s, dummy;
    float sampling_rate;

    csv_in.read_row(sampling_rate_s, sampling_rate, dummy, dummy);

    Log(fg(fmt::terminal_color::bright_green), "Sampling rate: {} Hz\n",
        sampling_rate);

    constexpr int skip2header = 7;
    for (int i = 0; i < skip2header; i++) {
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

    auto fft_x_ret = getFFTFreqMag(data_x, sampling_rate);
    auto fft_y_ret = getFFTFreqMag(data_y, sampling_rate);
    auto fft_z_ret = getFFTFreqMag(data_z, sampling_rate);

    /* sort them */
    sortByMag(fft_x_ret);

    /* Log to file */
    std::filesystem::create_directories(fft_log_path.parent_path());
    std::FILE* f_fft_log = fopen(fft_log_path.c_str(), "w");

    if (f_fft_log == nullptr) {
      throw std::runtime_error("fopen failed");
    }

    printFFTHeader(f_fft_log, sampling_rate);
    printFreqMag(f_fft_log, fft_x_ret);
    printFreqMag(f_fft_log, fft_y_ret);
    printFreqMag(f_fft_log, fft_z_ret);

  } catch (std::exception& e) {
    Log("{}\n", e.what());
  }

  /* do fft  */

  non_blocking_input.uiparser_.RegisterRcws(&rcws_instance);
  non_blocking_input.uiparser_.ListCmds();

  /*  */

  while (!non_blocking_input.GetExitFlag()) {
    non_blocking_input.ProcessInput();

    // take a reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
