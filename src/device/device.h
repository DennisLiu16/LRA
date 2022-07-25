#ifndef LRA_DEVICE_H_
#define LRA_DEVICE_H_

#include <memory/registers/registers.h>
#include <string>

namespace lra::device {

// concept template
using lra::memory::registers::is_register;

// struct and variant 
using ::lra::memory::registers::Register_8;
using ::lra::memory::registers::Register_16;
using ::lra::memory::registers::Register_32;
using ::lra::memory::registers::Register_64;
using ::lra::memory::registers::Register_T;

// abstract class
class Device {
 public:
  virtual std::string getDeviceID() = 0;
  virtual bool Init()= 0;
  virtual bool Connect() = 0;
  virtual bool Disonnect() = 0;
 // 應該用 concepts 避免繼承使用
};

}  // namespace lra::device_util
#endif