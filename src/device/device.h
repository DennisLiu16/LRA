#ifndef LRA_DEVICE_H_
#define LRA_DEVICE_H_

#include <memory/registers/registers.h>
#include <util/errors/errors.h>

namespace lra::device {

// notice
// You should check internal register address type by yourself (see tca.h)

// concept template
using ::lra::memory::registers::is_bitset;
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::is_valid_type_val;
using ::lra::memory::registers::same_as_dv;

// struct and variant
using ::lra::memory::registers::Register_8;
using ::lra::memory::registers::Register_16;
using ::lra::memory::registers::Register_32;
using ::lra::memory::registers::Register_64;
using ::lra::memory::registers::Register_T;

// using adapter and init struct
// I2C
using ::lra::bus_adapter::i2c::I2cAdapter;
using ::lra::bus_adapter::i2c::I2cAdapter_S;  // aka I2cAdapterInit_S

// errors
using ::std::to_underlying;
using ::lra::errors_util::Errors;

// defines
// CRITICAL: if iaddr_bytes set with wrong number, bus adapter will crush
// candidates:
// 1. I2C internal address convert function overrange (array)
struct I2cDeviceInfo {       // total: 8 bytes (2 byte alignment)
  bool tenbit_{false};       // I2C is 10 bit device address
  uint8_t iaddr_bytes_{1};   // I2C device internal(word) address bytes, such as: 24C04 1 byte, 24C64 2 bytes
                             // subaddress(register) len
                             // ref: https://ithelp.ithome.com.tw/m/articles/10274116
                             // ref: https://blog.csdn.net/euxnijuoh/article/details/53334323
  uint16_t addr_{0x0};       // I2C device(slave) address, 7 or 10 bits
  uint16_t page_bytes_{16};  // I2C max number of bytes per page, 1K/2K 8, "4K/8K/16K 16", 32K/64K 32 etc
  uint16_t flags_{0};        // I2C i2c_ioctl_read/write flags

  // TODO: make pre_work function (related to I2cDeviceinfo) to constexpr or consteval
  // link: https://www.youtube.com/watch?v=bIc5ZxFL198&t=375s&ab_channel=CppCon
  // how to make struct constexpr:
  // https://stackoverflow.com/questions/27408819/why-cant-non-static-data-members-be-constexpr
};

// util functions
template <typename V>
bool IsNegative(const V& v) {
  if constexpr (std::integral<V>) {
    return (v < 0);
  }

  // bitset will never evaluate as negative number
  return false;
}

}  // namespace lra::device
#endif