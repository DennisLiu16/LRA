#ifndef LRA_UTIL_LOG_H_
#define LRA_UTIL_LOG_H_

// see details at https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#LRA-log-system

// typeid issue
// ref:
// https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname

// test region

#if defined(USE_LOG_SYSTEM)
// Macro for spdlog, should placed above include spdlog files
#pragma message("Log system enable: use spdlog")

#else
#pragma message("Log system disable")
#endif

#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
// #include <source_location>  // require g++-11
#include <experimental/source_location>
#include <tuple>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

namespace lra_log_util {

class LogUnit {
 public:
  inline std::string getName() { return name_; }

  LogUnit(auto &obj) {  // input should be *this, an instance or nullptr
    // get name
    std::string class_name;
    if (!obj) {
      class_name = "global";
    } else {
      class_name = "class_" + boost::core::demangle(typeid(obj).name());
    }
    // increment idx_for_next_
    name_ = class_name.append(1, '_').append(std::to_string(idx_for_next_++));
    // important: https://zh-blog.logan.tw/2020/03/22/cxx-17-inline-variable/
  }

  ~LogUnit();

  static std::shared_ptr<LogUnit> CreateLogUnit(auto obj) {
    auto ptr = std::make_shared<LogUnit>(obj);
    Register(ptr);
    return ptr;
  }

  static std::shared_ptr<LogUnit> CreateLogUnit() { return CreateLogUnit(nullptr); }

  static bool IsRegistered(const std::string &logunit_name);
  static std::vector<std::string> getAllKeys();

  static auto getLogUnitPtr(const std::string &logunit_name) { return (IsRegistered(logunit_name)) ? logunits_[logunit_name] : nullptr; }

  void AddLogger(const std::string &logger_name);
  void RemoveLogger(const std::string &logger_name);
  bool Drop();
  static bool Drop(const std::string &logunit_name);

  // // deduction guide - failed
  // template <typename... Args>
  // struct Log {
  //   Log(spdlog::level::level_enum level, Args &&...args,
  //       const std::experimental::source_location &loc = std::experimental::source_location::current()) {
  //     if (SPDLOG_ACTIVE_LEVEL > level) {
  //       return;
  //     }

  //     for (const auto &logger : loggers_) {  // string type
  //       if (std::shared_ptr<spdlog::logger> log_ptr = spdlog::get(logger); log_ptr != nullptr) {
  //         (log_ptr)->log(spdlog::source_loc{loc.file_name(), loc.line(), loc.function_name()}, level, std::forward<Args>(args)...);
  //       } else {
  //         loggers_.erase(logger);
  //       }
  //     }
  //   }
  // };

  // // ref:
  // // https://cor3ntin.github.io/posts/variadic/
  // // https://stackoverflow.com/questions/57547273/how-to-use-source-location-in-a-variadic-template-function/57548488#57548488
  // // Deduction guide
  // template <typename... Args>
  // Log(spdlog::level::level_enum level, Args &&...) -> Log<Args...>;

  // example:
  // logunit->Log(std::make_tuple("msg id {}", 42), spdlog::level::error)
  // you should check SPDLOG_ACTIVE_LEVEL by CMakeLists
  // you can also set logger properties before RegisterLogger
  template <typename... Args>
  void Log(std::tuple<Args...> tuple, const spdlog::level::level_enum &level,
           const std::experimental::source_location &loc = std::experimental::source_location::current()) {
    if (SPDLOG_ACTIVE_LEVEL > level) {
      return;
    }

    for (const auto &logger : loggers_) {  // string type
      if (std::shared_ptr<spdlog::logger> log_ptr = spdlog::get(logger); log_ptr != nullptr) {
        std::apply([&](auto &&...args) { _CallSpdlog(log_ptr, level, loc, args...); }, tuple);
      } else {
        loggers_.erase(logger);
      }
    }
  }

 private:
  template <typename... Args>
  void _CallSpdlog(const std::shared_ptr<spdlog::logger> &log_ptr, const spdlog::level::level_enum &level,
                   const std::experimental::source_location &loc, Args &&...args) {
    (log_ptr)->log(spdlog::source_loc{loc.file_name(), loc.line(), loc.function_name()}, level, std::forward<Args>(args)...);
  }

  static uint32_t idx_for_next_;  // not static inlineD

  std::string name_;                         // unique LogUnit name
  std::unordered_set<std::string> loggers_;  // use string instead of spdlog::logger for
                                             // shared_ptr drop problem
  static std::unordered_map<std::string, std::shared_ptr<LogUnit>> logunits_;

  static void Register(std::shared_ptr<LogUnit> ptr);
};

}  // namespace lra_log_util
#endif