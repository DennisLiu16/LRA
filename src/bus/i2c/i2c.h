#ifndef LRA_BUS_I2C_H_
#define LRA_BUS_I2C_H_

#include <bus/bus.h>
#include <bus/i2c/i2c_sys.h>

#include <memory>
#include <string_view>

namespace lra::bus::i2c {

struct I2cInit_S {
  const char* bus_name_;
};

class I2c : public Bus<I2c> {
 public:
  // members
  int32_t fd_ = -1;
  const char* bus_name_;
  uint32_t speed_ = 100000;

  // CRTP Impl, using template
  bool InitImpl(const char* bus_name);

  bool InitImpl(const I2cInit_S& init_s);

  // you can declare something like
  // template <typename... Args>
  // ssize_t WriteImpl(const is_register auto& reg, Args... args);

  template <is_register T, typename U>
  requires is_valid_regOperation<T, U> ssize_t WriteImpl(const T& reg, const U& val) {
    // check val
    if (IsNegative(val)) {
      // TODO: Logerr: write to val is negative number?
      // ignore write 
      return 0;
    } 

    // i2c plain write
    // create buf
    sys::I2cIoctlWrite()
    // i2c smbus write
    i2c_smbus_write_byte();
  }

  template <typename... Args>
  ssize_t WriteMultiImpl(const is_register auto& reg_start, const uint32_t len, Args... args) {
    // How to check every value in container? -> write a self check concept function
  }

  template <typename... Args>
  ssize_t ReadImpl(const is_register auto& reg);

  template <typename... Args>
  ssize_t ReadMultiImpl(const is_register auto& reg_start, const uint32_t len, Args... args);

  template <typename... Args>
  ssize_t ModifyImpl(const is_register auto& reg, Args... args);

  // personal functions
  bool IsPiDistribution(std::string_view current_version);

  uint32_t getPiI2cSpeed();

  // TODO: Factory with shared_ptr

 private:
  bool IsPiDistribution(std::string_view);

  std::string getKernelVersion();

  uint32_t getPiI2cSpeed();

  ~I2c();
};

// struct
struct I2cDevice {                      // Restore an I2C device's properties and bus, 8 byte align, total 40 bytes
  uint32_t reserved_;                   // 4 bytes, reserved
  uint32_t delay_ = 100;                // 4 bytes, I2C internal operation delay, unit microseconds
  const char* dev_name_ = "";           // 8 bytes
  I2cDeviceInfo info_;                  // 8 bytes
  std::shared_ptr<I2c> bus_ = nullptr;  // 16 bytes,
};

struct I2cDeviceInfo {        // total: 8 bytes (2 bytes alignment)
  bool tenbit_ = 0;           // I2C is 10 bit device address
  uint8_t iaddr_bytes_ = 1;   // I2C device internal(word) address bytes, such as: 24C04 1 byte, 24C64 2 bytes
                              // subaddress(register) len
                              // ref: https://ithelp.ithome.com.tw/m/articles/10274116
  uint16_t page_bytes_ = 16;  // I2C max number of bytes per page, 1K/2K 8, 4K/8K/16K 16, 32K/64K 32 etc
  uint16_t addr_ = 0x0;       // I2C device(slave) address
  uint16_t flags_ = 0;        // I2C i2c_ioctl_read/write flags
};

// TODO: It's an example
// auto i2c_bus = make_shared<I2c>();
// i2c_bus->Init("/dev/i2c-1");
// DRV drv("x", struct) {

//   I2cDevice* dev_ptr = make_shared<I2cDevice>(struct);
//}

}  // namespace lra::bus::i2c

#endif
