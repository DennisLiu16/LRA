/*
 * File: fft_helper.hpp
 * Created Date: 2023-04-19
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:32:48 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include <fftw3.h>
#include <host_usb_lib/logger/logger.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/ostr.h>

#include <chrono>
#include <complex>
#include <numeric>
#include <span>
#include <utility>
#include <vector>

namespace lra::fft_lib {
namespace detail {
void removeMean(std::vector<float>& data, unsigned begin, unsigned len) {
  if (data.empty()) {
    return;
  }

  if (begin + len > data.size()) {
    throw std::out_of_range("Specified range is out of bounds.");
  }

  double sum =
      std::accumulate(data.begin() + begin, data.begin() + begin + len, 0.0);

  double mean = sum / len;

  for (unsigned i = begin; i < begin + len; ++i) {
    data[i] -= static_cast<float>(mean);
  }
}

void removeMean(std::vector<float>& data) {
  if (data.empty()) {
    return;
  }

  double sum = std::accumulate(data.begin(), data.end(), 0.0);

  double mean = sum / data.size();

  for (auto& element : data) {
    element -= static_cast<float>(mean);
  }
}

std::vector<std::pair<float, float>> getFFTFreqMag(
    std::span<const float> input_data, const float sampling_rate) {
  int N = input_data.size();

  fftw_complex *in, *out;
  in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
  out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

  for (int i = 0; i < N; ++i) {
    in[i][0] = input_data[i];
    in[i][1] = 0;
  }

  fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(plan);

  std::vector<std::pair<float, float>> freq_magnitude(N / 2 + 1);

  for (int i = 0; i < N / 2 + 1; ++i) {
    const float scaler = (i == 0) ? 1.0 : 2.0;
    float magnitude =
        scaler * std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / N;
    float frequency = i * sampling_rate / N;
    freq_magnitude[i] = std::make_pair(frequency, magnitude);
  }

  fftw_destroy_plan(plan);
  fftw_free(in);
  fftw_free(out);

  return freq_magnitude;
}
}  // namespace detail

void printFFTHeader(FILE* out, float sampling_rate) {
  auto now = std::chrono::system_clock::now();

  std::time_t current_time = std::chrono::system_clock::to_time_t(now);

  std::string formatted_time =
      fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(current_time));

  lra::usb_lib::Log(out, "FFT Record\n");
  lra::usb_lib::Log(out, "Current time: {}\n", formatted_time);
  lra::usb_lib::Log(out, "Sample Rate: {} (Hz)\n\n", sampling_rate);
}

void printFreqMag(FILE* out, std::vector<std::pair<float, float>>& in) {
  std::string col1_name = "Frequency(Hz)";
  std::string col2_name = "Magnitude";
  const auto reserve_length =
      (col1_name.length() > 20) ? col1_name.length() : 20;

  lra::usb_lib::Log(out, "{1:^{0}}, {2:^{0}}\n", reserve_length, col1_name,
                    col2_name);

  for (const auto& [freq, mag] : in) {
    lra::usb_lib::Log(out, "{1:^{0}.4f}, {2:^{0}.4f}\n", reserve_length, freq,
                      mag);
  }

  /* end of fft log */
  auto format = fmt::format("{0:*^{1}}", "End of FFT", 2 * reserve_length);
  lra::usb_lib::Log(out, "\n\n\n{}\n\n\n", format);
  std::fflush(out);
}

void printFreqMag(std::vector<std::pair<float, float>>& in) {
  std::string col1_name = "Frequency(Hz)";
  std::string col2_name = "Magnitude";
  const auto reserve_length =
      (col1_name.length() > 20) ? col1_name.length() : 20;

  lra::usb_lib::Log("{1:^{0}}, {2:^{0}}\n", reserve_length, col1_name,
                    col2_name);

  for (const auto& [freq, mag] : in) {
    lra::usb_lib::Log("{1:^{0}.4f}, {2:^{0}.4f}\n", reserve_length, freq, mag);
  }
  auto format = fmt::format("{0:*^{1}}", "End of FFT", 3 * reserve_length);
  lra::usb_lib::Log("\n\n\n{}\n\n\n", format);
  std::fflush(stdout);
}

void sortByMag(std::vector<std::pair<float, float>>& data) {
  if (data.empty()) return;

  std::sort(
      data.begin(), data.end(),
      [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
        return a.second > b.second;
      });
}

/* important: this function will remove mean */
std::vector<std::pair<float, float>> getFFTFreqMag(
    std::vector<float>& input_data, float sampling_rate) {
  detail::removeMean(input_data);
  return detail::getFFTFreqMag(std::span{input_data.data(), input_data.size()},
                               sampling_rate);
}

/* TODO: FFTW plan reuse  */
/* important: this function will remove mean */
std::vector<std::pair<float, float>> getFFTFreqMag(
    std::vector<float>& input_data, float sampling_rate, unsigned begin,
    unsigned len) {
  detail::removeMean(input_data, begin, len);
  return detail::getFFTFreqMag(std::span{input_data.data() + begin, len},
                               sampling_rate);
}

}  // namespace lra::fft_lib
