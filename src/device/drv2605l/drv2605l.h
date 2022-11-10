#ifndef LRA_DEVICE_DRV2605L_H_
#define LRA_DEVICE_DRV2605L_H_

#include <bus_adapter/i2c_adapter/i2c_adapter.h>
#include <device/device.h>
#include <device/device_info.h>
#include <memory/registers/registers.h>
#include <util/log/logunit.h>

namespace lra::device {
using ::lra::bus_adapter::i2c::I2cAdapter;
using ::lra::bus_adapter::i2c::I2cAdapter_S;
using ::lra::log_util::loglevel;

// ref: device/tca

// most cared registers information
struct Drv2605lInfo {
  bool diag_result_{1};  // default failure
  bool over_current_detect_{1};
  bool over_temp_detect_{1};
  float vbat_{0.0};
  float lra_freq_{0.0};  // hz
  float compensation_coeff_{0.0};
  float back_emf_result_{0.0};
  std::string device_id_{""};
};

struct Drv2605lRtInfo {
  uint8_t rtp_{0};
  float lra_freq_{0.0};
};

class Drv2605l {  // DRV2605L
 public:
  I2cDeviceInfo info_;
  std::shared_ptr<lra::log_util::LogUnit> logunit_;

  // constructor
  Drv2605l() = delete;
  Drv2605l(const I2cDeviceInfo& info, std::string name);

  bool Init(I2cAdapter_S init_s);

  // Write
  template <is_register T, typename U>
    requires same_as_dv<T, U> || std::integral<U>
  ssize_t Write(const T& reg, const U val) {
    if (IsNegative(val)) {
      logunit_->LogToDefault(loglevel::err, "drv name: {} write an negative value!", name_);
      return std::to_underlying(Errors::kWrite);
    }

    if constexpr (is_register<U>)
      return adapter_.Write(reg, val.to_ullong());
    else
      return adapter_.Write(reg, val);
  }

  ssize_t Write(const uint8_t addr, const uint8_t* val, const uint32_t len);

  template <typename T>
    requires std::same_as<T, std::vector<uint8_t>> || std::convertible_to<T, uint8_t>
  ssize_t Write(const uint8_t& addr, const T& val) {
    // val will never be negative number
    // Write(const uint8_t&, const std::vector<uint8_t>& / const uint8_t& )
    if constexpr (std::convertible_to<T, uint8_t>)
      return adapter_.Write(addr, (uint8_t)val);
    else
      return adapter_.Write(addr, val);
  }

  ssize_t Write(const uint8_t addr, const std::initializer_list<uint8_t>& list) {
    const std::vector<uint8_t> vec = list;
    return Write(addr, vec);
  }

  // Read
  template <is_register T>
  auto Read(const T& reg) {
    typename T::val_t val = 0;
    return (adapter_.Read(reg, val) == reg.bytelen_) ? val : std::to_underlying(Errors::kRead);
  }

  int16_t Read(const uint8_t addr);

  ssize_t Read(const uint8_t addr, uint8_t* val, const uint16_t len);

  std::vector<uint8_t> Read(const uint8_t addr, const uint16_t len);

  // TODO: add modify

  // registers
  constexpr static Register_8 STATUS{0x0, 0xE0};
  constexpr static Register_8 MODE{0x1, 0x40};
  constexpr static Register_8 RTP_INPUT{0x02, 0x00};
  constexpr static Register_8 LIBRARY_SELECTION{0x03, 0x01};
  constexpr static Register_8 WAVEFORM_SEQUENCER{0x04, 0x01};  // array
  constexpr static Register_8 GO{0x0C, 0x00};
  constexpr static Register_8 ODT{0x0D, 0x00};
  constexpr static Register_8 SPT{0x0E, 0x00};
  constexpr static Register_8 SNT{0x0F, 0x00};
  constexpr static Register_8 BRT{0x10, 0x00};
  constexpr static Register_8 ATV_CONTROL{0x11, 0x05};
  constexpr static Register_8 ATV_MINIMUM_INPUT{0x12, 0x19};
  constexpr static Register_8 ATV_MAXIMUM_INPUT{0x13, 0xFF};
  constexpr static Register_8 ATV_MINIMUM_OUTPUT{0x14, 0x19};
  constexpr static Register_8 ATV_MAXIMUM_OUTPUT{0x15, 0xFF};
  constexpr static Register_8 RATED_VOLTAGE{0x16, 0x3E};
  constexpr static Register_8 OD_CLAMP{0x17, 0x8C};
  constexpr static Register_8 A_CAL_COMP{0x18, 0x0C};  // Auto-Calibration Compensation Result
  constexpr static Register_8 A_CAL_BEMF{0x19, 0x6C};
  constexpr static Register_8 FEEDBACK_CONTROL{0x1A, 0x36};
  constexpr static Register_8 CONTROL1{0x1B, 0x93};
  constexpr static Register_8 CONTROL2{0x1C, 0xF5};
  constexpr static Register_8 CONTROL3{0x1D, 0xA0};
  constexpr static Register_8 CONTROL4{0x1E, 0x20};
  constexpr static Register_8 CONTROL5{0x1F, 0x80};
  constexpr static Register_8 OL_LRA_PERIOD{0x20, 0x33};
  constexpr static Register_8 VBAT{0x21, 0x00};
  constexpr static Register_8 LRA_PERIOD{0x22, 0x00};

  // functions
  void SetAllReg(std::vector<uint8_t> v);

  std::vector<uint8_t> GetAllReg();

  void UpdateAllReg(std::vector<uint8_t> v);

  void SetToLraDefault();

  Drv2605lInfo GetCalibrationInfo();

  Drv2605lInfo RunAutoCalibration();

  void UpdateRTP(uint8_t val);

  float GetHz();

  Drv2605lRtInfo GetRt();

  void Run(bool);  // fire go or stop

  void Ready(bool);  // let device get into ready state or software standby

  inline bool GetRun() { return run_; }

 private:
  I2cAdapter adapter_;
  std::string name_;
  bool run_{false};
};
}  // namespace lra::device

#endif