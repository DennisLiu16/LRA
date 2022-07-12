#ifndef LRA_DEVICE_BUS_H_
#define LRA_DEVICE_BUS_H_

#include <device/registers/registers.h>
#include <util/concepts/concepts.h>

namespace lra::device::bus {
using ::lra::device::registers::IsRegister;

template <typename T>
class Bus {
 public:
  Bus() = default;
  ~Bus() = default;


  // TODO: write must use explicit data type e.g. long long to avoid implicit conversion
  template <typename... Args>
  bool WriteImpl(const IsRegister auto& reg, Args... args);

  template <typename... Args>
  bool WriteMultiImpl(const IsRegister auto& reg_begin, const IsRegister auto& reg_end, Args... args);

  template <typename... Args>
  auto ReadImpl(const IsRegister auto& reg);

  template <typename... Args>
  bool ReadMultiImpl(const IsRegister auto& reg, Args... args);


};

class I2C {

};

}  // namespace lra::device::bus

#endif