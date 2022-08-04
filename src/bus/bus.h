#ifndef LRA_BUS_H_
#define LRA_BUS_H_

#include <memory/registers/registers.h>
#include <util/concepts/concepts.h>
#include <util/log/log.h>
#include <util/terminal/terminal.h>

// TODO: include fnctl ...

namespace lra::bus {
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::is_valid_type_val;

// classes

template <typename T>
class Bus {
 public:
  std::shared_ptr<lra::log_util::LogUnit> logunit_ = nullptr;  // try to eliminate this

  // CRTP interface

  // make Method enum in every bus class, and pass them as first template param

  template <typename... Args>
  bool Init(Args&&... args) {
    return static_cast<T*>(this)->InitImpl(std::forward<Args>(args)...);  // more general form
  }

  template <auto Method, typename... Args>
  ssize_t Write(Args&&... args) {
    return static_cast<T*>(this)->WriteImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t WriteMulti(Args&&... args) {
    return static_cast<T*>(this)->WriteMultiImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t Read(Args&&... args) {
    return static_cast<T*>(this)->ReadImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t ReadMulti(Args&&... args) {
    return static_cast<T*>(this)->ReadMultiImpl<Method>(std::forward<Args>(args)...);
  }

 protected:
  static inline bool FdValid(int fd) { return fcntl(fd, F_GETFL) != -1 || errno != EBADF; };

 private:
  Bus() = default;
  friend T;
};
}  // namespace lra::bus
#endif