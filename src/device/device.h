#ifndef LRA_DEVICE_H_
#define LRA_DEVICE_H_

#include <device/registers/registers.h>
#include <vector>

namespace lra::device {

using lra::device::registers::IsRegister;

class Device {
 public:
  std::vector<> reg;

};

}  // namespace lra::device_util
#endif