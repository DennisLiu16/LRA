#ifndef LRA_BUS_I2C_H_
#define LRA_BUS_I2C_H_
#include <arpa/inet.h>
#include <bus/bus.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

namespace lra::bus {

struct I2cInit_S {
  const char* name_;
};

// corresponding type for smbus to i2c_rdwr_ioctl_data
struct i2c_rdwr_smbus_data {
  uint8_t command_{0x0};   // internal register address, smbus only allow __u8
  uint8_t len_{0};         // max 32
  uint8_t slave_addr_{0};  // only allow 7-bit address
  uint8_t* value_{nullptr};
};

class I2c : public Bus<I2c> {
 public:
  // vars
  int32_t fd_{-1};
  int32_t func_{-1};   // from ioctl(fd, I2C_FUNC, &func_);
  uint32_t speed_{0};  // 0 for unknown, invalid
  const char* name_{""};

  // enum
  enum class I2cMethod { kPlain, kSmbus };

 private:
  // let base class becomes friend to use impl func
  friend Bus;

  // CRTP Impl, call private i2c core functions
  bool InitImpl(const char* bus_name);

  bool InitImpl(const I2cInit_S& init_s);

  // write "single" byte
  template <I2cMethod method>
  ssize_t WriteImpl(const void* data);

  template <I2cMethod method>
  ssize_t WriteMultiImpl(const void* data);

  // read "single" byte
  template <I2cMethod method>
  ssize_t ReadImpl(void* data);

  template <I2cMethod method>
  ssize_t ReadMultiImpl(void* data);

  // vars
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

  ssize_t PlainRW(i2c_rdwr_ioctl_data* data);

  template <bool Read>
  ssize_t SmbusRW(i2c_rdwr_smbus_data* data);

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
  int32_t I2cUpdateThisBusFunc();

  /**
   * @brief Update and compare
   *
   * @param device
   * @param func
   * @return true
   * @return false
   */
  bool I2cUpdateFuncAndCompare(int32_t func);

  // speed related
  bool IsPiDistribution(std::string_view current_version);

  std::string getKernelVersion();

  uint32_t UpdateI2cSpeedOnPi();

  ~I2c();
};

}  // namespace lra::bus

#endif
