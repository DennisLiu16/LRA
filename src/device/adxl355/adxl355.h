#ifndef LRA_DEVICE_ADXL355_H_
#define LRA_DEVICE_ADXL355_H_

// TODO: 待 spi_adapter 和 spi_bus 寫好後重構

#include <device/device.h>
#include <memory/registers/registers.h>
#include <util/log/logunit.h>

#include <array>
#include <deque>

// tmp
extern "C" {
#include <wiringPi.h>
#include <wiringPiSPI.h>
}

namespace lra::device {

using ::lra::log_util::loglevel;

// tmp struct for spi channel
struct SpiInit_s {
  int mode_{0};
  int channel_{0};
  int speed_{10000000};
};

class Adxl355 {
 public:
  struct Acc3 {  // 16 bytes
    Float3 data;
    float time;
  };

  // States
  bool standby_{false};
  int range_{0b10};  // 4 g

  const float dRange_2g = 2.048 * 2;
  const float dRange_4g = 4.096 * 2;
  const float dRange_8g = 8.192 * 2;

  // Register
  constexpr static Register_8 DEVID_AD{0x00, 0xAD};
  constexpr static Register_8 DEVID_MST{0x01, 0x1D};
  constexpr static Register_8 PARTID{0x02, 0xED};
  constexpr static Register_8 DREVID{0x03, 0x01};
  constexpr static Register_8 Status{0x04, 0x00};
  constexpr static Register_8 FIFO_ENTRIES{0x05, 0x00};
  constexpr static Register_8 TEMP2{0x06, 0x00};
  constexpr static Register_8 TEMP1{0x07, 0x00};
  constexpr static Register_8 XDATA3{0x08, 0x00};
  constexpr static Register_8 XDATA2{0x09, 0x00};
  constexpr static Register_8 XDATA1{0x0A, 0x00};
  constexpr static Register_8 YDATA3{0x0B, 0x00};
  constexpr static Register_8 YDATA2{0x0C, 0x00};
  constexpr static Register_8 YDATA1{0x0D, 0x00};
  constexpr static Register_8 ZDATA3{0x0E, 0x00};
  constexpr static Register_8 ZDATA2{0x0F, 0x00};
  constexpr static Register_8 ZDATA1{0x10, 0x00};
  constexpr static Register_8 FIFO_DATA{0x11, 0x00};
  constexpr static Register_8 OFFSET_X_H{0x1E, 0x00};
  constexpr static Register_8 OFFSET_X_L{0x1F, 0x00};
  constexpr static Register_8 OFFSET_Y_H{0x20, 0x00};
  constexpr static Register_8 OFFSET_Y_L{0x21, 0x00};
  constexpr static Register_8 OFFSET_Z_H{0x22, 0x00};
  constexpr static Register_8 OFFSET_Z_L{0x23, 0x00};
  constexpr static Register_8 ACT_EN{0x24, 0x00};
  constexpr static Register_8 ACT_THRESH_H{0x25, 0x00};
  constexpr static Register_8 ACT_THRESH_L{0x26, 0x00};
  constexpr static Register_8 ACT_COUNT{0x27, 0x01};
  constexpr static Register_8 Filter{0x28, 0x00};
  constexpr static Register_8 FIFO_SAMPLES{0x29, 0x60};
  constexpr static Register_8 INT_MAP{0x2A, 0x00};
  constexpr static Register_8 Sync{0x2B, 0x00};
  constexpr static Register_8 Range{0x2C, 0x81};
  constexpr static Register_8 POWER_CTL{0x2D, 0x01};
  constexpr static Register_8 SELF_TEST{0x2E, 0x00};
  constexpr static Register_8 Reset{0x2F, 0x00};

  // Register pool
  constexpr static ::std::array regs_{::std::to_array<Register_T>(
      {DEVID_AD, DEVID_MST,    PARTID,       DREVID,     Status,     FIFO_ENTRIES, TEMP2,      TEMP1,
       XDATA3,   XDATA2,       XDATA1,       YDATA3,     YDATA2,     YDATA1,       ZDATA3,     ZDATA2,
       ZDATA1,   FIFO_DATA,    OFFSET_X_H,   OFFSET_X_L, OFFSET_Y_H, OFFSET_Y_L,   OFFSET_Z_H, OFFSET_Z_L,
       ACT_EN,   ACT_THRESH_H, ACT_THRESH_L, ACT_COUNT,  Filter,     FIFO_SAMPLES, INT_MAP,    Sync,
       Range,    POWER_CTL,    SELF_TEST,    Reset})};

  // Init and IO
  Adxl355() = default;

  void Init(SpiInit_s init, std::string name);

  ssize_t Write(const uint8_t addr, const uint8_t *val, const uint16_t len);

  std::vector<uint8_t> Read(const uint8_t addr, const uint16_t len);

  // Functions
  std::string CheckDeviceReg();

  void SetToDefault();

  // void SetSamplingRate();

  // void SetAccRange(int);

  void SetStandBy(bool);

  void UpdateAllReg(std::vector<uint8_t> v);

  // Acc3 GetAccRange();

  void SetOffSet(Acc3);

  Acc3 GetOffSet();

  // working
  std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> GetAllReg();

  Acc3 GetAcc();

  // int GetSamplingRate();

  // bool GetStandBy();

  Acc3 AccPopFront();

  std::vector<Acc3> AccPopFrontN(size_t n);

  std::vector<Acc3> AccPopAll();

  void AccPushBack(Acc3 acc_data);

  inline ssize_t GetDataDequeSize() { return data_.size(); }

 private:
  std::deque<Acc3> data_{};
  std::shared_ptr<lra::log_util::LogUnit> logunit_{nullptr};
  std::mutex dq_mutex_{};
  std::string name_{};
  SpiInit_s init_{};

  // abort
  std::mutex rw_mutex_{};  // FIXME: only one adxl355 can work on same spi bus

  // functions
  Acc3 ParseDigitalAcc(std::vector<uint8_t> v);
  float GetCacheRange();
};
}  // namespace lra::device

#endif