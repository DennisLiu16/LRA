#include <util/log/log.h>

#include <iostream>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

std::string ptr2_name;

void func() {
  std::shared_ptr<lra_log_util::LogUnit> ptr2 = lra_log_util::LogUnit::CreateLogUnit();
  // if a lot of logunit will generate -> remember to use Drop() deregister the logunit
  ptr2_name = ptr2->getName();
  ptr2->Drop(); // good
}

int main() {
  std::shared_ptr<lra_log_util::LogUnit> ptr = lra_log_util::LogUnit::CreateLogUnit();
  func();

  // TODO: 把 logunits 放到 private 建立 interface
  if(auto ptr2_ = lra_log_util::LogUnit::getLogUnitPtr(ptr2_name); ptr2_ != nullptr)
    spdlog::fmt_lib::print("{}\n", ptr2_.use_count());  

  auto my_logger = spdlog::basic_logger_mt("console", "/home/ubuntu/LRA/data/log/log_test.log");
  my_logger->set_level(spdlog::level::err);
  //spdlog::stdout_color_mt("console");

  ptr->AddLogger("console");
  int i,j = 0;
  while(i++ < 4000) {
    ptr->Log(std::make_tuple("test"), spdlog::level::err);
  }

  spdlog::set_default_logger(spdlog::get("console"));

  while(j++ < 4000) {
    spdlog::error("test");
  }

}