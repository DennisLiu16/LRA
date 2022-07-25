#ifndef LRA_UTIL_TERMINAL_H_
#define LRA_UTIL_TERMINAL_H_

#include <string>

namespace lra::terminal_util {
std::string Execute(const char* cmd);
std::string getKernelVersion();
std::string getOs();

}

#endif