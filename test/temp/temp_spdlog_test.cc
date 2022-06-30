// define macro here to test
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <set>

namespace fs = std::filesystem;

// the relation of filesystem and spdlog::filename_t (spdlog::filename_t == std::string)
fs::path parent_dir = "/home/ubuntu/LRA/data/log";
fs::path file_name_func = "func_basic_log.log";
fs::path file_name_main = "main_basic_log.log";
auto file_for_func = (parent_dir / file_name_func).string();
auto file_for_main = (parent_dir / file_name_main).string();

void GenerateLoggerInFunction() {
  // add handle here
  spdlog::file_event_handlers handlers;
  handlers.before_open = [](spdlog::filename_t filename) { spdlog::info("Before opening {}", filename); };
  handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) { fputs("After opening\n", fstream); };
  handlers.before_close = [](spdlog::filename_t filename, std::FILE *fstream) { fputs("Before closing\n", fstream); };
  handlers.after_close = [](spdlog::filename_t filename) { spdlog::info("After closing {}", filename); };
  // create logger
  auto log = spdlog::basic_logger_st("func_basic_logger", file_for_func, true, handlers);
}

void PrintAllLogger() {
  static std::set<std::string> myset;
  static uint32_t num = 0;
  spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { myset.insert(l->name()); num++; });

  // print all
  spdlog::fmt_lib::print("All logger --\n");
  for (auto s : myset) {
    spdlog::fmt_lib::print("{}\n", s);
  }
  spdlog::fmt_lib::print("Total logger num: {}\n\n", num);
}

int main() {
  spdlog::basic_logger_mt("main_basic_logger", file_for_main);
  GenerateLoggerInFunction();

  // get all registered logger
  PrintAllLogger();

  // log something in func logger
  auto log_func = spdlog::get("func_basic_logger");
  log_func->set_level(spdlog::level::trace);
  log_func->trace("This is a trace msg");
  log_func->debug("This is a debug msg");
  log_func->info("This is a info msg");
  log_func->warn("This is a warn msg");
  log_func->error("This is a error msg");
  log_func->critical("This is a critical msg");
  log_func->info("---------------------");

  // global logger and macro test
  spdlog::set_level(spdlog::level::trace);
  spdlog::debug("func - debug");
  SPDLOG_TRACE("MACRO - TRACE");
  SPDLOG_DEBUG("MACRO - DEBUG");
  SPDLOG_INFO("MACRO - INFO");
  SPDLOG_WARN("MACRO - WARN");
  SPDLOG_ERROR("MACRO - ERROR");
  SPDLOG_CRITICAL("MACRO - CRITICAL");

  // macro of specified logger
  SPDLOG_LOGGER_TRACE(log_func, "MACRO - TRACE");
  SPDLOG_LOGGER_DEBUG(log_func, "MACRO - DEBUG");
  SPDLOG_LOGGER_INFO(log_func, "MACRO - INFO");
  SPDLOG_LOGGER_WARN(log_func, "MACRO - WARN");
  SPDLOG_LOGGER_ERROR(log_func, "MACRO - ERROR");
  SPDLOG_LOGGER_CRITICAL(log_func, "MACRO - CRITICAL");
}

// conclusion
// 1. 在 func 可以建立 logger 並可以用 apply_all 查詢 https://github.com/gabime/spdlog/issues/259
// 2. CMake 中 target 與 lib spdlog 是否由進行函式連結對 macro 的影響
// 3. default logger 是沒有名字的, 具顏色輸出到 stdout