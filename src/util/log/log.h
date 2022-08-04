#ifndef LRA_UTIL_LOG_H_
#define LRA_UTIL_LOG_H_

// see details at https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#LRA-log-system
// - source_location as default param in varadic template
// - demangle name problem
// - log with fmt should use spdlog::format_string_t<Args...> to convert cosst char*

#include <spdlog/spdlog.h>

#include <boost/core/demangle.hpp>
// #include <source_location>  // require g++-11
#include <experimental/source_location>
#include <tuple>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

// TODO: V2 - make a simpler(use more spdlog features) and string_view args version

namespace lra::log_util {

struct LevelAndLocation {
  spdlog::level::level_enum level;
  std::experimental::source_location loc;

  LevelAndLocation(spdlog::level::level_enum lev,
                   const std::experimental::source_location &l = std::experimental::source_location::current())
      : level(lev), loc(l){}
};

class LogUnit {
 public:
  inline std::string getName() { return name_; }

  LogUnit(const char* name);

  LogUnit(const auto &obj) {  // input should be *this, an instance or nullptr
    // get name
    std::string class_name;
    if constexpr (std::is_same<decltype(obj), decltype(nullptr) const &>::value) {  // type determine is nullptr const&
      class_name = "global";
    } else {
      class_name = "class_" + boost::core::demangle(typeid(obj).name());
    }
    // increment idx_for_next_
    name_ = class_name.append(1, '_').append(std::to_string(idx_for_next_++));
  }

  ~LogUnit();

  static std::shared_ptr<LogUnit> CreateLogUnit(const auto &obj) {  // auto obj
    auto ptr = std::make_shared<LogUnit>(obj);
    Register(ptr);
    ApplyDefaultLogger(ptr);
    return ptr;
  }

  static std::shared_ptr<LogUnit> CreateLogUnit() { return CreateLogUnit(nullptr); }

  static bool IsRegisteredLogUnit(const std::string &logunit_name);
  static void AddDefaultLogger(const std::string &logger);
  static void RemoveDefaultLogger(const std::string &logger);
  static std::unordered_set<std::string> getAllDefaultLoggers();
  static std::vector<std::string> getAllLogunitKeys();  // for registered logunits

  static auto getLogUnitPtr(const std::string &logunit_name) {
    return (IsRegisteredLogUnit(logunit_name)) ? logunits_[logunit_name] : nullptr;
  }

  void AddLogger(const std::string &logger_name);
  void RemoveLogger(const std::string &logger_name);
  bool Drop();
  static bool Drop(const std::string &logunit_name);

  // // ref:
  // // https://cor3ntin.github.io/posts/variadic/

  // example:
  // logunit->Log(std::make_tuple("msg id {}", 42), spdlog::level::error)

  // 7/12 find this can't compile owing to std::make_tuple require constexpr since g++-10
  // compile pass - g++-9.4.0 clang++10.0.0 -- aborted

  //   template <typename... Args>
  //   void Log(std::tuple<Args...> tuple, const spdlog::level::level_enum &level,
  //            const std::experimental::source_location &loc = std::experimental::source_location::current()) {
  //     if (SPDLOG_ACTIVE_LEVEL > level) {
  //       return;
  //     }

  //     for (const auto &logger : loggers_) {  // string type
  //       if (std::shared_ptr<spdlog::logger> log_ptr = spdlog::get(logger); log_ptr != nullptr) {
  //         std::apply([&](auto &&...args) { _CallSpdlog(log_ptr, level, loc, args...); }, tuple);
  //       } else {
  //         loggers_.erase(logger);
  //       }
  //     }
  //   }

  //  private:
  //   template <typename... Args>
  //   void _CallSpdlog(const std::shared_ptr<spdlog::logger> &log_ptr, const spdlog::level::level_enum &level,
  //                    const std::experimental::source_location &loc,spdlog::format_string_t<Args...> fmt, Args &&...
  //                    args) {
  //     std::string new_function_name(loc.function_name());

  //     (log_ptr)->log(spdlog::source_loc{loc.file_name(), (int)loc.line(), (new_function_name += " <" + this->name_ +
  //     ">").c_str()},
  //                    level, fmt, std::forward<Args>(args)...);
  //   }

  /**
   *  @brief A method for logunit to access spdlog::logger->log(...)
   *
   *  @tparam  LevelAndLocation    Type contains spdlog::level_enum and source_location information
   *  @param   info                Param stores level and where this function be called
   *  @param   fmt                 Param stores format string.
   *  @param   args                Rest of format string arguments
               
      @details New version with struct LevelAndLocation to set source location to default argument.
               failed). Can't ignore fmt will do compile time check (requires constexpr).
               const char* will lead compiling error: args#0 is not constant expression.
               spdlog::format_string_t<Args...> will complete the transformation.
               You should check SPDLOG_ACTIVE_LEVEL by CMakeLists. You can also set logger properties before
               RegisterLogger.

      @htmlinclude https://stackoverflow.com/a/66402319/17408307
   */
  template <typename... Args>
  void Log(LevelAndLocation info, spdlog::format_string_t<Args...> fmt, Args &&...args) {
    if (SPDLOG_ACTIVE_LEVEL > info.level) {
      return;
    }

    for (const auto &logger : loggers_) {  // string type
      if (std::shared_ptr<spdlog::logger> log_ptr = spdlog::get(logger); log_ptr != nullptr) {
        std::string new_function_name(info.loc.function_name());
        (log_ptr)->log(spdlog::source_loc{info.loc.file_name(), (int)info.loc.line(),
                                          (new_function_name += " <" + this->name_ + ">").c_str()},
                       info.level, fmt, std::forward<Args>(args)...);
      } else {
        loggers_.erase(logger);
      }
    }
  }

  // TODO: We need a Log, LogAll and LogGlobal

 private:
  static uint32_t idx_for_next_;  // not static inline, https://zh-blog.logan.tw/2020/03/22/cxx-17-inline-variable/

  std::string name_;                         // unique LogUnit name
  std::unordered_set<std::string> loggers_;  // use string instead of spdlog::logger for
                                             // shared_ptr drop problem
  static std::unordered_map<std::string, std::shared_ptr<LogUnit>> logunits_;
  static std::unordered_set<std::string> default_loggers_;

  static void Register(std::shared_ptr<LogUnit> ptr);
  static void ApplyDefaultLogger(std::shared_ptr<LogUnit> ptr);
};

}  // namespace lra::log_util
#endif