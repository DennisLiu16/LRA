#ifndef LRA_BUS_H_
#define LRA_BUS_H_

#include <memory/registers/registers.h>
#include <util/concepts/concepts.h>
#include <util/log/log.h>
#include <util/terminal/terminal.h>

namespace lra::bus {
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::is_valid_regOperation;

// classes

template <typename T>
class Bus {
 public:
  std::shared_ptr<lra::log_util::LogUnit> logunit_ = nullptr;

  // CRTP
  template <typename S>
  bool Init(const S& init_s) {
    return static_cast<T*>(this)->InitImpl(init_s);  // specify different init struct for different bus, or const char*
  }

  template <typename... Args>
  ssize_t Write(Args... args) {
    return static_cast<T*>(this)->WriteImpl(args...);
  }

  template <typename... Args>
  ssize_t WriteMulti(Args... args) {
    return static_cast<T*>(this)->WriteMultiImpl(args...);
  }

  template <typename... Args>
  ssize_t Read(Args... args) {
    return static_cast<T*>(this)->ReadImpl(args...);
  }

  template <typename... Args>
  ssize_t ReadMulti(Args... args) {
    return static_cast<T*>(this)->ReadMultiImpl(args...);
  }

  template <typename... Args>
  ssize_t Modify(Args... args) {
    return static_cast<T*>(this)->ModifyImpl(args...);
  }

 protected:
  template <typename V>
  bool IsNegative(const V& v) {
    if constexpr (std::integral<V>) {
      return (v < 0);
    }

    // bitset will never evaluate as negative number
    return false;
  }

  // Impl
  // template <typename S>
  // bool InitImpl(const uint64_t& addr, const S& init_s);

  // template <typename... Args>
  // bool WriteImpl(const is_register auto& reg, Args... args);

  // template <typename... Args>
  // bool WriteMultiImpl(const is_register auto& reg_begin, const is_register auto& reg_end, Args... args);

  // template <typename... Args>
  // auto ReadImpl(const is_register auto& reg);

  // template <typename... Args>
  // bool ReadMultiImpl(const is_register auto& reg_begin, const is_register auto& reg_end, Args... args);

 private:
  Bus() = default;
  friend T;
};
}  // namespace lra::bus
#endif