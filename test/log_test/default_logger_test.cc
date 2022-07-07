#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <util/log/log.h>
#include <util/timer/timer.h>

#include <iostream>

int main() {
  auto my_logger = spdlog::basic_logger_mt("console", "/home/ubuntu/LRA/data/log/log_test.log", true);
  my_logger->set_level(spdlog::level::trace);
  my_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%!:%#] %v");

  lra::log_util::LogUnit::AddDefaultLogger("console");

  auto vec = lra::log_util::LogUnit::getAllLogunitKeys();
  for(auto s: vec) {
    spdlog::fmt_lib::print("{}\n", s);
  }

  lra::timer_util::Timer T;
}