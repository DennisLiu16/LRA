#include <array>
#include <memory>

namespace lra::terminal_util {
std::string Execute(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::string getOs() {
  return Execute("cat /etc/os-release");
}

std::string getKernelVersion() {
  return Execute("uname -r");
}
}  // namespace lra::util_terminal