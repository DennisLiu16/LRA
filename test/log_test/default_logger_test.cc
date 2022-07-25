#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <util/log/log.h>
#include <util/timer/timer.h>

#include <iostream>

int main() {
  auto my_logger = spdlog::basic_logger_mt("console", "/home/ubuntu/LRA/data/log/log_test.log", true);
  my_logger->set_level(spdlog::level::trace);
  my_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%!:%#] %v");

  // test AddDefaultLogger working situation of multiple thread class
  lra::log_util::LogUnit::AddDefaultLogger("console");
  lra::timer_util::Timer T;
  auto vec = lra::log_util::LogUnit::getAllLogunitKeys();
  for(auto s: vec) {
    spdlog::fmt_lib::print("{}\n", s);
  }
  

  spdlog::fmt_lib::print("----------\n");

  // test const char* as logunit name 
  auto log_ptr = lra::log_util::LogUnit::CreateLogUnit("my_logunit");
  vec = lra::log_util::LogUnit::getAllLogunitKeys();
  for(auto s: vec) {
    spdlog::fmt_lib::print("{}\n", s);
  }
}