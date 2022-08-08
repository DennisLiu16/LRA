#ifndef LRA_DEVICE_TCA9548A_H_
#define LRA_DEVICE_TCA9548A_H_

#include <bus_adapter/i2c_adapter/i2c_adapter.h>
#include <device/device.h>
#include <memory/registers/registers.h>

// SUM
// 1. has dynamic type at compile time by using typename T::typedef ...
// 2. don't use reference in integral type args in first layer interface

namespace lra::device {

class Tca9548a {
 public:
  I2cDeviceInfo info_;  // constructor device_info -> then Init adapter

  // constructor
  Tca9548a() = delete;
  Tca9548a(const I2cDeviceInfo& info);

  // functions

  /**
   * @brief copy a init struct to init i2c adapter
   *
   * @return true
   * @return false
   */
  bool Init(I2cAdapter_S init_s);

  /**
   * @brief write single register
   *
   * @tparam T Register_T
   * @tparam U bitset / integral
   * @param reg Register_T
   * @param val type restrict to integral / same_as<decltype(reg.dv_)>
   * @return ssize_t (successed) ? 1 : error_code
   */
  template <is_register T, typename U>  // not <..., std::integral U>
  requires same_as_dv<T, U> || std::integral<U> ssize_t Write(const T& reg, const U val) {
    if (IsNegative(val)) {  // restrict value not negative
      // TODO: LogGlobal(spdlog::level::debug, "reg val: {} is negative\n", );
      return std::to_underlying(Errors::kWrite);
    }

    // perfect forward to adapter write interface
    // XXX:should always convert to correct type if you want to use bitset (it's related to tmp_buf size -> may crush)
    // It's ok to pass oversize val now (check in i2c_adapter layer), that is we don't need to write something like 
    // ret = adapter_.Write(reg, (tpyename T::val_t)val);

    if constexpr (is_register<U>)
      return adapter_.Write(reg, val.to_ullong());
    else
      return adapter_.Write(reg, val);
  }

  // TODO: V2 - write multiple registers, e.g. Write(start_reg, {bitset<8>, bitset<64>})

  // write 1 or multiple bytes that starts from addr in array form
  // return n bytes
  ssize_t Write(const uint8_t addr, const uint8_t* val, const uint32_t len);

  // write 1 byte to addr with val (uint8_t) or multiple bytes that starts from addr in std::vector<uint8_t> form
  template <typename T>
  requires std::same_as<T, std::vector<uint8_t>> || std::convertible_to<T, uint8_t> ssize_t Write(const uint8_t& addr,
                                                                                                  const T& val) {
    // val will never be negative number
    // Write(const uint8_t&, const std::vector<uint8_t>& / const uint8_t& )
    return adapter_.Write(addr, std::convertible_to<T, uint8_t> ? (uint8_t)val : val);
  }

  // initializer_list has no type so it's not allowed to deduced in template. We must use initializer_list to avoid
  // deduced
  // FIXME: can't use (reg.addr, {0x1,0x2}); -> const uint8_t& to const uint8_t
  template <std::integral T>
  ssize_t Write(const uint8_t addr, const std::initializer_list<T>& list) {
    const std::vector<uint8_t> vec = list;
    return Write(addr, vec);
  }

  // read one register (<= 8 bytes)
  // if ret size > 0 -> get the register val
  // use auto to get val
  template <is_register T>
  auto Read(const T& reg) {
    typename T::val_t val = 0;
    return (adapter_.Read(reg, val) == reg.bytelen_)
               ? val
               : std::to_underlying(Errors::kRead);  // read bytelen == setting len
  }

  // read one byte from addr
  int16_t Read(const uint8_t addr);

  // read multiple bytes to an array, or use vector.data()
  // val type: uint8_t or vector<uint8_t>& -> create a vector and reserve to len
  ssize_t Read(const uint8_t addr, uint8_t* val, const uint16_t len);

  // user overloading (optional)
  // RVO only works if you call:
  // auto result = Read(addr, len);
  // in your source code
  std::vector<uint8_t> Read(const uint8_t addr, const uint16_t len);

  // single byte modify, device define
  // ssize_t Modify(const uint8_t addr, const uint8_t val, const uint8_t mask);

  // registers
  constexpr static Register_8 CONTROL{0x0, 0x0};

 private:
  I2cAdapter adapter_;
};
}  // namespace lra::device

#endif