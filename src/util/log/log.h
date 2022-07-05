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
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif

#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace lra_log_util {

class LogUnit {
 public:
  inline static std::unordered_map<std::string, std::shared_ptr<LogUnit>> logunits_{};

  // LogManager::RegisterLogUnit(getName()); the
  // name is unique so no need to set string;
  inline std::string getName() { return name_; }

  LogUnit(auto &obj) {
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
    return std::move(ptr);
  }

  static std::shared_ptr<LogUnit> CreateLogUnit() { return CreateLogUnit(nullptr); }
  static bool IsRegistered(std::string logunit_name);

  void AddLogger(std::string logger_name);
  void RemoveLogger(std::string logger_name);
  void Drop();


  template <typename... Args>
  void Log(int level, Args &&...args) {

    // level check
    if(!(SPDLOG_ACTIVE_LEVEL > level)) {return;}

    for (const auto &logger : loggers_) { // string type
      if (std::shared_ptr<spdlog::logger> log_ptr = spdlog::get(logger); log_ptr != nullptr) {
        switch (level) {
          case spdlog::level::trace:
            SPDLOG_LOGGER_TRACE(log_ptr, std::forward<Args>(args)...);
            break;
          case spdlog::level::debug:
            SPDLOG_LOGGER_DEBUG(log_ptr, std::forward<Args>(args)...);
            break;
          case spdlog::level::info:
            SPDLOG_LOGGER_INFO(log_ptr, std::forward<Args>(args)...);
            break;
          case spdlog::level::warn:
            SPDLOG_LOGGER_WARN(log_ptr, std::forward<Args>(args)...);
            break;
          case spdlog::level::err:
            SPDLOG_LOGGER_ERROR(log_ptr, std::forward<Args>(args)...);
            break;
          case spdlog::level::critical:
            SPDLOG_LOGGER_CRITICAL(log_ptr, std::forward<Args>(args)...);
            break;
        }
      } else {  // remove logger because it's not valid anymore
        loggers_.erase(logger);
      }
    }
  }

 private:
  static uint32_t idx_for_next_; // not static inline

  std::string name_;                         // unique LogUnit name
  std::unordered_set<std::string> loggers_;  // use string instead of spdlog::logger for
                                             // shared_ptr drop problem

  static void Register(std::shared_ptr<LogUnit> ptr);
};

}  // namespace lra_log_util
#endif