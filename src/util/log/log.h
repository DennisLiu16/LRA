#ifndef LRA_UTIL_LOG_H_
#define LRA_UTIL_LOG_H_

// see details at https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#LRA-log-system

// typeid issue
// ref: https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname

// test region

#if defined(USE_LOG_SYSTEM)
// Macro for spdlog, should placed above include spdlog files

#pragma message("Log system enable: use spdlog")
// #define LOG_TRACE(,...)

#else
#pragma message("Log system disable")
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif

#include <spdlog/spdlog.h>

namespace lra_log_util {

class LogManager {
 public:
  LogManager() = default;
  ~LogManager() = default;

  // apply log to all loggers - spdlog
  // log to stdout - stdlog
  void Log();

  void Log2();
};

}  // namespace lra_log_util
#endif