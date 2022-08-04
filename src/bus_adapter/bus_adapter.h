#ifndef LRA_BUS_ADAPTER_H_
#define LRA_BUS_ADAPTER_H_

#include <bus/bus.h>
#include <memory/registers/registers.h>
#include <unistd.h>

namespace lra::bus_adapter {
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::Register_T;

template <typename T>
class BusAdapter {
 public:
  std::shared_ptr<lra::log_util::LogUnit> logunit_ = nullptr;

  // CRTP interface
  // template <typename S>
  // bool Init(const S& init_s) {
  //   return static_cast<T*>(this)->InitImpl(init_s);  // specify different init struct for different bus, or const
  //   char*
  // }

  template <typename... Args>
  bool Init(Args&&... args) {
    return static_cast<T*>(this)->InitImpl(std::forward<Args>(args)...);  // more general
  }

  template <typename... Args>
  ssize_t Write(Args&&... args) {
    return static_cast<T*>(this)->WriteImpl(std::forward<Args>(args)...);
  }

  // use auto to enable us to return different type such as uintx_t
  template <typename... Args>
  auto Read(Args&&... args) {
    return static_cast<T*>(this)->ReadImpl(std::forward<Args>(args)...);
  }

 protected:
  // some useful functions

  // limits to 8 bytes
  auto MakeMaskWithLen(uint8_t byte_len) {
    uint64_t mask = 0;
    if (byte_len >= 8) return (~mask);
    while (byte_len--) {
      mask = mask << 8 | 0x0ff;
    }
    return mask;
  }

  // transfer uint8_t array to related integral type (in big-endian)
  // e.g. buf[4]={0x11,0x22,0x33,0x44} -> int(0x11223344)
  // CRITICAL: array len should always align with type U
  template <std::integral U>
  U Array2Integral_BE(U& val, uint8_t* buf) {
    for (int i = 0; i < sizeof(U); i++) {
      val = (val << 8) | buf[i];
    }
    return val;
  }

  // transfer uint8_t array to related integral type (in little-endian)
  // e.g. buf[4]={0x11,0x22,0x33,0x44} -> int(0x44332211)
  // CRITICAL: array len should always align with type U
  template <std::integral U>
  U Array2Integral_LE(U& val, uint8_t* buf) {
    for (int i = 0; i < sizeof(U); i++) {
      val = (val << 8) | buf[sizeof(U) - i - 1];
    }
    return val;
  }

 private:
  BusAdapter() = default;
  friend T;
};
}  // namespace lra::bus_adapter

#endif