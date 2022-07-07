#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <util/log/log.h>

#include <iostream>

std::string ptr2_name;
std::shared_ptr<lra_log_util::LogUnit> global_ptr;

void func() {
  std::shared_ptr<lra_log_util::LogUnit> ptr2 = lra_log_util::LogUnit::CreateLogUnit();
  ptr2_name = ptr2->getName();
}

int main() {
  std::shared_ptr<lra_log_util::LogUnit> ptr = lra_log_util::LogUnit::CreateLogUnit();
  func();

  // TODO: 把 logunits 放到 private 建立 interface
  if (auto ptr2_ = lra_log_util::LogUnit::getLogUnitPtr(ptr2_name); ptr2_ != nullptr) spdlog::fmt_lib::print("{}\n", ptr2_.use_count());

  auto my_logger = spdlog::basic_logger_mt("console", "/home/ubuntu/LRA/data/log/log_test.log", true);
  my_logger->set_level(spdlog::level::trace);

  auto vec = lra_log_util::LogUnit::getAllKeys();
  for(auto s: vec) {
    spdlog::fmt_lib::print("{}\n", s);
  }

  ptr->AddLogger("console");
  int i = 0, j = 0;
  while (i++ < 4000) {
    ptr->Log(std::make_tuple("{}:test", i), spdlog::level::trace);
  }

  spdlog::set_default_logger(spdlog::get("console"));

  while (j++ < 4000) {
    spdlog::trace("test");  
  }
}