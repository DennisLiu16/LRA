#include <util/log/log.h>

#include <iostream>

#include <spdlog/sinks/stdout_color_sinks.h>

std::string ptr2_name;

void func() {
  std::shared_ptr<lra_log_util::LogUnit> ptr2 = lra_log_util::LogUnit::CreateLogUnit();
  // if a lot of logunit will generate -> remember to use Drop() deregister the logunit
  ptr2_name = ptr2->getName();
  ptr2->Drop(); // good
}

int main() {
  int a;
  std::shared_ptr<lra_log_util::LogUnit> ptr = lra_log_util::LogUnit::CreateLogUnit();
  func();

  // TODO: 把 logunits 放到 private 建立 interface
  if(lra_log_util::LogUnit::logunits_.count(ptr2_name))
    spdlog::fmt_lib::print("{}\n", (lra_log_util::LogUnit::logunits_[ptr2_name].use_count()));
  for (auto k : lra_log_util::LogUnit::logunits_) {
    std::cout << k.first << " " << k.second.use_count() << std::endl;
  }

  spdlog::stdout_color_mt("console");

  ptr->AddLogger("console");
  ptr->Log(spdlog::level::err, "test");
  ptr->Log(spdlog::level::err, "test");

}