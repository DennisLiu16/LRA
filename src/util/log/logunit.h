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

#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
#include <experimental/source_location>  // <--> <source_location>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <string_view>

namespace lra::log_util {

using loglevel = spdlog::level::level_enum;

class LogUnit {
 public:
  // public dtor
  ~LogUnit();

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

  // public functions
  void AddLogger(const std::string &logger_name);
  void RemoveLogger(const std::string &logger_name);
  bool Drop(const std::string& logunit_name = "");
  std::string getName();
  std::shared_ptr<LogUnit> getLogUnit(const std::string &str);

  template <typename... Args>
  void LogToDefault(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if (SPDLOG_ACTIVE_LEVEL > ll.level_) return;

    std::string new_function_name = spdlog::fmt_lib::format("{} | <{}>", ll.loc_.function_name(), this->name_);

    // default logger's default value is color stdout: see spdlog/registry-inl.h
    spdlog::default_logger()->log(spdlog::source_loc{ll.loc_.file_name(), (int)ll.loc_.line(), new_function_name.c_str()},
                                ll.level_, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void LogToLoggers(const Level_Loc &ll, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if ((SPDLOG_ACTIVE_LEVEL > ll.level_) || (loggers_.size() == 0)) return;

    std::string new_function_name = spdlog::fmt_lib::format("{} | <{}>", ll.loc_.function_name(), this->name_);

    // log -> spdlog::log_it_ has constant qualifier (), forward should be ok
    for (const auto &logger_name : loggers_) {
      if (auto logger = spdlog::get(logger_name); logger != nullptr) {
        logger->log(spdlog::source_loc{ll.loc_.file_name(), (int)ll.loc_.line(), new_function_name.c_str()}, ll.level_,
                    fmt, std::forward<Args>(args)...);
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
  static std::unordered_map<std::string, std::shared_ptr<LogUnit>> logunits_;

  // public variables

 private:
  // private ctor's
  // 1. "" -> default
  // 2. s_... -> usr input string
  // 3. {class_name} -> class object
  LogUnit();
  LogUnit(const char* cstr);
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
};

}  // namespace lra::log_util

#endif