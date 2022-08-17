#ifndef LRA_BUS_I2C_H_
#define LRA_BUS_I2C_H_
#include <arpa/inet.h>
#include <bus/bus.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#ifdef __cplusplus
}
#endif

#include <memory>
#include <string_view>

#ifdef I2C_UNIT_TEST
#include <spdlog/spdlog.h>
#endif

namespace lra::bus {

struct I2cInit_S {
  const char* name_;
};

// corresponding type for smbus to i2c_rdwr_ioctl_data
struct i2c_rdwr_smbus_data {
  bool no_internal_reg_{false};
  uint8_t command_{0x0};   // internal register address, smbus only allow __u8
  uint8_t len_{0};         // max 32
  uint8_t slave_addr_{0};  // only allow 7-bit address
  uint8_t* value_{nullptr};
};

class I2c : public Bus<I2c> {
 public:
  // vars
  int32_t fd_{-1};
  uint64_t func_{0};   // from ioctl(fd, I2C_FUNC, &funcs); // uint32_t
  uint32_t speed_{0};  // 0 for unknown, invalid
  const char* name_{""};

  // enum
  enum class I2cMethod { kPlain, kSmbus };

  ~I2c();

 private:
  // let base class becomes friend to use impl func
  friend Bus;

  // CRTP Impl, call private i2c core functions
  bool InitImpl(const char* bus_name);

  bool InitImpl(const I2cInit_S& init_s);

  // write "single" byte
  template <I2cMethod method>
  ssize_t WriteImpl(const void* data) {
    return WriteMultiImpl<method>(data);
  }

  template <I2cMethod method>
  ssize_t WriteMultiImpl(const void* data) {
    if constexpr (I2cMethod::kPlain == method) {  // I2C plain device, prefer

      auto typed_data = static_cast<const i2c_rdwr_ioctl_data*>(data);
      return PlainRW(typed_data);

    } else if constexpr (I2cMethod::kSmbus == method) {  // I2C SMBUS

      auto typed_data = static_cast<const i2c_rdwr_smbus_data*>(data);
      return SmbusRW<false>(typed_data);  // false -> write cmd
    }
  }

  template <I2cMethod method>
  ssize_t ReadImpl(void* data) {
    return ReadMultiImpl<method>(data);
  }

  template <I2cMethod method>
  ssize_t ReadMultiImpl(void* data) {
    if constexpr (I2cMethod::kPlain == method) {  // I2C plain device, prefer

      auto typed_data = static_cast<i2c_rdwr_ioctl_data*>(data);
      return PlainRW(typed_data);

    } else if constexpr (I2cMethod::kSmbus == method) {  // I2C SMBUS

      auto typed_data = static_cast<i2c_rdwr_smbus_data*>(data);
      return SmbusRW<true>(typed_data);  // write for false
    }
  }

  // vars
  // I2C_SLAVE is independent for each open fd:
  // https://stackoverflow.com/questions/41172797/linuxs-i2c-dev-interface-with-multiple-processes
  uint16_t last_slave_addr_{0};

  // i2c core function

  /**
   * @brief i2c open function
   *
   * @param bus_name
   * @return int
   * @details You can use i2c-tools to check i2c buses on your machine
   *          (i2cdetect -l)
   */
  int I2cOpen(const char* bus_name);

  void I2cClose(int fd);

  void I2cResetThisBus();

  template <typename T>
  requires std::same_as<std::remove_const_t<T>, i2c_rdwr_ioctl_data> ssize_t PlainRW(T* data) {
#ifdef I2C_UNIT_TEST
    spdlog::fmt_lib::print("In PlainRW\n");
    spdlog::fmt_lib::print("nmsg: {}\n", data->nmsgs);
    spdlog::fmt_lib::print("addr: {}\n", data->msgs[data->nmsgs - 1].addr);
    spdlog::fmt_lib::print("flags: {}\n", data->msgs[data->nmsgs - 1].flags);
    spdlog::fmt_lib::print("len: {}\n", data->msgs[data->nmsgs - 1].len);
    spdlog::fmt_lib::print("buf: ");
    if (data->nmsgs == 2) {  // read
      // generate fake data
    }
    for (uint16_t i = 0; i < data->msgs[data->nmsgs - 1].len; i++) {
      spdlog::fmt_lib::print("{} ", *((data->msgs[data->nmsgs - 1].buf) + i));
    }
    spdlog::fmt_lib::print("\n\n");
    return data->msgs[data->nmsgs - 1].len;
#else
    if (ioctl(fd_, I2C_RDWR, (unsigned long)data) < 0) {
      return -1;
    }

    // return msg[1].len for iaddr read and msg[0].len for non-iaddr-read / write
    // write len == iaddr_bytes + size
    // read len  == size
    // FIXME: no iaddr case?
    return data->msgs[data->nmsgs - 1].len;
#endif
  }

  // https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/
  /* Until kernel 2.6.22, the length is hardcoded to 32 bytes. If you
     ask for less than 32 bytes, your code will only work with kernels
     2.6.23 and later. */
  // SMBUS failed or should not use:
  // 1. page->size > 32 && len > 32
  // 2. no internal address but more than one register (len != 1)
  template <bool Read, typename T>
  requires std::same_as<std::remove_const_t<T>, i2c_rdwr_smbus_data> ssize_t SmbusRW(T* data) {
    if (last_slave_addr_ != data->slave_addr_) {  // change to target slave device
      if (ioctl(fd_, I2C_SLAVE, data->slave_addr_) < 0) return -1;
      last_slave_addr_ = data->slave_addr_;
    }

    if constexpr (Read) {  // read
      // Assume no internal address -> only one register
      if (data->no_internal_reg_) {
        if (data->len_ == 1) {
          __s32 val = i2c_smbus_read_byte(fd_);
          if (val > -1) *(data->value_) = val;
          return (val > -1) ? 1 : val /*errno*/;
        } else
          assert(data->len_ == 1 && "More than one byte read for no-internal-address register by smbus i2c, waiting for impl");
      } else
        return i2c_smbus_read_i2c_block_data(fd_, data->command_, data->len_, data->value_);

    } else {  // write
      // Assume no internal address -> only one register
      if (data->no_internal_reg_) {
        if (data->len_ == 1) {
          __s32 val = i2c_smbus_write_byte(fd_, *(data->value_));
          return (val > -1) ? 1 : val /*errno*/;
        } else
          assert(data->len_ == 1 && "More than one byte write for no-internal-address register by smbus i2c, waiting for impl");

      } else
        return i2c_smbus_write_i2c_block_data(fd_, data->command_, data->len_, data->value_);
    }
  }

  // i2c sub functions

  /**
   * @brief
   *
   * @param device
   * @param log default to false
   * @return int
   * @url: https://elixir.bootlin.com/linux/v5.4/source/include/uapi/linux/i2c.h#L90
   * @url: https://www.kernel.org/doc/html/latest/i2c/functionality.html
   */
  uint64_t I2cUpdateThisBusFunc();

  /**
   * @brief Update and compare
   *
   * @param device
   * @param func
   * @return true
   * @return false
   */
  bool I2cUpdateFuncAndCompare(uint64_t func);

  // speed related
  bool IsPiDistribution(std::string_view current_version);

  std::string getKernelVersion();

  uint32_t UpdateI2cSpeedOnPi();
};

}  // namespace lra::bus

#endif
