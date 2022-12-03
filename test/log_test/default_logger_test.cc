#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <util/log/logunit.h>
#include <util/timer/timer.h>

#include <chrono>
using lra::log_util::loglevel;

void test() { auto log_ptr = lra::log_util::LogUnit::CreateLogUnit("In_func"); }

int main() {
  fmt::print("spdlog level: {}\n\n", SPDLOG_ACTIVE_LEVEL);

  const char* log_path = "/home/ubuntu/LRA/data/log/log_test.log";
  const char* simple_log_path = "/home/ubuntu/LRA/data/log/simple_log_test.log";

  // create multiple sinks
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [ %! ] %v");

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);
  file_sink->set_level(spdlog::level::trace);
  file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [ %! ] %v");

  // you must set logger's level to trace to enable sink's level(will be filted here first, then sink's level)
  auto multiple_sink_logger =
      std::make_shared<spdlog::logger>("multi_sink", std::initializer_list<spdlog::sink_ptr>{console_sink, file_sink});
  multiple_sink_logger->set_level(loglevel::trace);

  spdlog::set_default_logger(multiple_sink_logger);

  // simple logger
  auto simple_logger = spdlog::basic_logger_st("simple_logger", simple_log_path, true);
  simple_logger->set_pattern("%v");
  simple_logger->set_level(loglevel::info);

  // udp logger

  // test AddDefaultLogger working situation of multiple thread class
  lra::timer_util::Timer T;
  lra::timer_util::Timer T2;

  auto vec = lra::log_util::LogUnit::getAllLogUnitKeys();
  for (auto i : vec) {
    fmt::print("{}\n", i);
  }

  spdlog::fmt_lib::print("----------\n");

  std::string test_only{"my_logunit"};

  // test const char* as logunit name
  auto log_ptr = lra::log_util::LogUnit::CreateLogUnit(test_only);
  auto log_ptr2 = lra::log_util::LogUnit::CreateLogUnit();
  std::string ss{"str_view"};
  std::string_view str_view{ss};
  auto log_ptr3 = lra::log_util::LogUnit::CreateLogUnit(str_view);

  auto start = std::chrono::high_resolution_clock::now();
  // write to default
  int cycle = 100000;
  for (int i = 0; i < cycle; ++i) {
    // log_ptr->LogToDefault(loglevel::critical, "{}", i);
    log_ptr->LogToDefault(loglevel::trace, "{}",
                          i);  // test cmake splog macro working or not --> set to warn (should not print)
  }
  auto end = std::chrono::high_resolution_clock::now();
  fmt::print("cost time: {:.5f} (ms)\n", (end - start).count() / 1e6);

  test();

  auto ptr = lra::log_util::LogUnit::getLogUnit("s_In_Func_0");

  vec = lra::log_util::LogUnit::getAllLogUnitKeys();
  for (auto i : vec) {
    fmt::print("{}\n", i);
  }

  // apply logger
  log_ptr->AddLogger(simple_logger);
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < cycle; ++i) {
    log_ptr->LogToLoggers(loglevel::critical, "{}, {}, {}, {}", i, i, i, i);
    // log_ptr->LogToLoggers(loglevel::debug, "{}, {}, {}, {}", i, i, i, i);
  }
  end = std::chrono::high_resolution_clock::now();

  fmt::print("cost time: {:.5f} (ms)\n", (end - start).count() / 1e6);

  // you need to drop logger and logunit
  lra::log_util::LogUnit::DropAllLogUnits();
  spdlog::drop_all();
}