#ifndef LRA_UTIL_LOG_H_
#define LRA_UTIL_LOG_H_
#include <spdlog/spdlog.h>

// use cmake to determine log stream
// ref: https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
#ifdef LRA_LOG_TO_STDOUT
#include <spdlog/sinks/stdout_color_sinks.h>

#endif
#endif