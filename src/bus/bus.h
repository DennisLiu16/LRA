#ifndef LRA_BUS_H_
#define LRA_BUS_H_

#include <assert.h>
#include <fcntl.h>
#include <memory/registers/registers.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <util/concepts/concepts.h>
#include <util/terminal/terminal.h>

// TODO: include fnctl ...

namespace lra::bus {
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::is_valid_type_val;

// classes

template <typename T>
class Bus {
 public:
  // CRTP interface

  // make Method enum in every bus class, and pass them as first template param

  template <typename... Args>
  bool Init(Args&&... args) {
    return static_cast<T*>(this)->InitImpl(std::forward<Args>(args)...);  // more general form
  }

  // Important: use template keyword make Method valid
  // keyword "template" in template function explicitly declare the method is a template class function
  // Without template, Method is unknown util the template function instantiates, leading compile error
  template <auto Method, typename... Args>
  ssize_t Write(Args&&... args) {
    return static_cast<T*>(this)->template WriteImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t WriteMulti(Args&&... args) {
    return static_cast<T*>(this)->template WriteMultiImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t Read(Args&&... args) {
    return static_cast<T*>(this)->template ReadImpl<Method>(std::forward<Args>(args)...);
  }

  template <auto Method, typename... Args>
  ssize_t ReadMulti(Args&&... args) {
    return static_cast<T*>(this)->template ReadMultiImpl<Method>(std::forward<Args>(args)...);
  }

 protected:
  static inline bool FdValid(int fd) { return fcntl(fd, F_GETFL) != -1 || errno != EBADF; };

 private:
  Bus() = default;
  friend T;
};
}  // namespace lra::bus
#endif