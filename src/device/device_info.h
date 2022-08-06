#ifndef LRA_DEVICE_INFO_H_
#define LRA_DEVICE_INFO_H_
namespace lra::device {

// store all Deviceinfo for specified bus

// I2C defines
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
}
#endif