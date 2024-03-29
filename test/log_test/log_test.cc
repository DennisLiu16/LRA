#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <util/log/logunit.h>

#include <iostream>

using lra::log_util::loglevel;

std::string ptr2_name;
std::shared_ptr<lra::log_util::LogUnit> global_ptr;

// multiple sinks: https://spdlog.docsforge.com/v1.x/2.creating-loggers/#creating-loggers-with-multiple-sinks

void func() {
  std::shared_ptr<lra::log_util::LogUnit> ptr2 = lra::log_util::LogUnit::CreateLogUnit();
  ptr2_name = ptr2->getName();
}

int main() {
  std::shared_ptr<lra::log_util::LogUnit> ptr = lra::log_util::LogUnit::CreateLogUnit();
  func();

  // TODO: 把 logunits 放到 private 建立 interface
  if (auto ptr2_ = lra::log_util::LogUnit::getLogUnit(ptr2_name); ptr2_ != nullptr)
    spdlog::fmt_lib::print("{}\n", ptr2_.use_count());

  auto my_logger = spdlog::basic_logger_mt("console", "/home/ubuntu/LRA/data/log/log_test.log", true);
  my_logger->set_level(spdlog::level::trace);
  my_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%!:%#] %v");



  ptr->AddLogger("console");
  int i = 0, j = 0;
  while (i++ < 4000) {
    ptr->LogToAll(loglevel::trace, "{}:test", i);
  }

  spdlog::set_default_logger(spdlog::get("console"));

  while (j++ < 4000) {
    spdlog::trace("test");
  }
}