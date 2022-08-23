#ifndef LRA_UTIL_LOGUNIT_H_
#define LRA_UTIL_LOGUNIT_H_

/**
 * @brief This file is for class Logunit and related spdlog custom flag functions (internal class)
 * @link [spdlog custom flag](https://spdlog.docsforge.com/v1.x/3.custom-formatting/)
 * @link [lra logunit](https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#LRA-log-system)
 *
 * @note We have solved
 *       1. source_location as default param in varadic template
 *       2. demangle variable using boost::core::demangle
 *       3. spdlog args change: fmt v8 requires compile time check (FMT_CONSTEVAL),
 *          leading const char* or string_view type args generating compile error
 *          -> use spdlog::format_string_t<Args...> instead.
 *
 */

#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
#include <experimental/source_location>  // <--> <source_location>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

namespace lra::log_util {

using loglevel = spdlog::level::level_enum;

class LogUnit {
 public:
  // public static functions
  template <typename... Arg>
  static std::shared_ptr<LogUnit> CreateLogUnit(Arg &&...arg) {
    // Access private ctor through class member struct CtorAccessor
    struct CtorAccessor : public LogUnit {
      CtorAccessor(Arg &&...arg) : LogUnit(std::forward<Arg>(arg)...){};
    };

    auto ptr = std::make_shared<CtorAccessor>(std::forward<Arg>(arg)...);
    Register(ptr);
    return ptr;
  }

  // public struct
  // Internal struct for source location generating without macro, record log level and src location.

  struct Level_Loc {
    loglevel level_;
    std::experimental::source_location loc_;

    Level_Loc(loglevel lev, const std::experimental::source_location &l = std::experimental::source_location::current())
        : level_(lev), loc_(l) {}

    Level_Loc(const Level_Loc &LL) : level_(LL.level_), loc_(LL.loc_) {}
  };

  // public static functions
  static std::shared_ptr<LogUnit> getLogUnit(const std::string &str);
  static std::vector<std::string> getAllLogUnitKeys();
  static ssize_t RefreshKeys();

  // public functions
  void AddLogger(const std::string &logger_name);
  void AddLogger(const std::shared_ptr<spdlog::logger> logger_ptr);
  void AddLogger(const spdlog::logger &logger);
  void RemoveLogger(const std::string &logger_name);
  void RemoveLogger(const std::shared_ptr<spdlog::logger> logger_ptr);
  void RemoveLogger(const spdlog::logger &logger);

  bool Drop(const std::string &logunit_name = "");
  std::string getName();

  // critical
  static void DropAllLogUnits();

  template <typename... Args>
  void LogToDefault(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if (SPDLOG_ACTIVE_LEVEL > ll.level_) return;

    auto default_logger = spdlog::default_logger();
    if ((default_logger != nullptr) && (default_logger->level() > ll.level_)) return;

    const char *new_function_name = spdlog::fmt_lib::format("{} | {}", ll.loc_.function_name(), this->name_).c_str();

    // default logger's default value is color stdout: see spdlog/registry-inl.h

    default_logger->log(spdlog::source_loc{ll.loc_.file_name(), (int)ll.loc_.line(), new_function_name}, ll.level_, fmt,
                        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void LogToLoggers(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if ((SPDLOG_ACTIVE_LEVEL > ll.level_) || (loggers_.size() == 0)) return;

    const char *new_function_name = spdlog::fmt_lib::format("{} | {}", ll.loc_.function_name(), this->name_).c_str();

    // log -> spdlog::log_it_ has constant qualifier (), forward should be ok
    for (const auto &logger_name : loggers_) {
      if (auto logger = spdlog::get(logger_name); logger != nullptr) {
        if (logger->level() > ll.level_) continue;

        logger->log(spdlog::source_loc{ll.loc_.file_name(), (int)ll.loc_.line(), new_function_name}, ll.level_, fmt,
                    std::forward<Args>(args)...);
      } else {
        loggers_.erase(logger_name);
      }
    }
  }

  // log to spdlog::default and loggers_
  template <typename... Args>
  void LogToAll(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    // warning
    LogToDefault(ll, fmt, std::forward<Args>(args)...);
    LogToLoggers(ll, fmt, std::forward<Args>(args)...);
  }

  // if loggers_ is empty, log to spdlog::default_logger
  template <typename... Args>
  void LogToExist(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    // warning
    loggers_.size() ? LogToLoggers(ll, fmt, std::forward<Args>(args)...)
                    : LogToDefault(ll, fmt, std::forward<Args>(args)...);
  }

  // public static variables

  // public variables

 private:
  // private ctor's
  // 1. "" -> default
  // 2. s_... -> usr input string
  // 3. {class_name} -> class object
  LogUnit();
  LogUnit(const char *cstr);
  LogUnit(const std::string_view str_v);
  LogUnit(const std::string &name);

  LogUnit(const auto &obj) {
    std::string class_name = boost::core::demangle(typeid(obj).name()).append("}");
    name_ = CreateLogUnitName(class_name.insert(0, "{"));
  }

  // private static functions
  static void Register(std::shared_ptr<LogUnit> ptr);

  // private functions
  std::string CreateLogUnitName(const std::string &prefix);

  // private member variables
  std::string name_{""};
  std::unordered_set<std::string> loggers_{};

  // private static variables
  static std::unordered_map<std::string, std::shared_ptr<LogUnit>> logunits_;
};

}  // namespace lra::log_util

#endif