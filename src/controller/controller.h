#ifndef LRA_CONTROLLER_H_
#define LRA_CONTROLLER_H_

#include <bus/i2c/i2c.h>
#include <device/adxl355/adxl355.h>
#include <device/drv2605l/drv2605l.h>
#include <device/tca/tca.h>
#include <util/log/logunit.h>

#include <chrono>
#include <thread>

extern "C" {
#include <wiringPi.h>
}

namespace lra::controller {

using ::lra::bus::I2c;
using ::lra::bus_adapter::i2c::I2cAdapter_S;
using ::lra::device::Adxl355;
using ::lra::device::Drv2605l;
using ::lra::device::Drv2605lInfo;
using ::lra::device::Drv2605lRtInfo;
using ::lra::device::I2cDeviceInfo;
using ::lra::device::SpiInit_s;
using ::lra::device::Tca9548a;
using ::lra::log_util::loglevel;
using ::lra::log_util::LogUnit;


class Controller {  // FIXME: only one controller allows, for static function callback sake
 public:
  // const
  const uint8_t drv_x_ch_{0x08};  // ch3
  const uint8_t drv_y_ch_{0x10};  // ch4
  const uint8_t drv_z_ch_{0x80};  // ch7
  const ssize_t max_number_in_deque_{1024 * 1024 * 5 /
                                     sizeof(Adxl355::Acc3)};  // constrains to 5 MB => (1024 / 16) * 1024 * 5

  // states
  bool adxl355_measure_thread_exit_{false};
  // bool on_calibration_{false};   // 應該是一個mutex, 掌管 disable input and ouput 或說不更新的功能 > 移到 main.cc
  // bool on_modify_{false};        // 應該是一個mutex, 掌管 disable input 或說不更新的功能 > 移到 main.cc
  std::thread adxl355_measure_t_{};
  std::chrono::system_clock::time_point start_time_{};

  // bus & device info
  I2c i2c_;
  I2cAdapter_S i2c_adapter_s_;

  I2cDeviceInfo info_tca_;
  I2cDeviceInfo info_drv_;
  SpiInit_s info_adxl_;

  // members
  std::shared_ptr<Drv2605l> drv_x_{nullptr};
  std::shared_ptr<Drv2605l> drv_y_{nullptr};
  std::shared_ptr<Drv2605l> drv_z_{nullptr};
  std::shared_ptr<Adxl355> adxl_{nullptr};

  // callbacks
  static bool new_acc_data_;
  static void ItCallback();

  // functions
  Controller();
  ~Controller();

  void Init();

  void RunDrv();

  void PauseDrv();

  void AccMeasureTask();

  void UpdateAllRtp(std::tuple<uint8_t, uint8_t, uint8_t>);

  void UpdateRtp(uint8_t, char);

  std::tuple<Drv2605lInfo, Drv2605lInfo, Drv2605lInfo, Adxl355::Acc3> RunCalibration();

  void ChangeDrvCh(char);

  void CancelMeasureTask();

  void StartMeasureTask();

  std::tuple<Drv2605lRtInfo,Drv2605lRtInfo ,Drv2605lRtInfo> GetRt();

  // 當有 require 來到時要被 called
  // void onModify();  // all, drv_x, drv_y, drv_z, adxl
  /**
   * include all disable
   * 1. RunCalibration
   * 2. UpdateAllRegisters
   * 3. GetAllRegisters
   *
   *
   * dividual
   * 1. GetOffset of acc
   * 2. UpdateRegisters
   * 3. 沒有單獨拿一個的 -> 拿全部 // FIXME
   *
   * not include
   * 1. UpdateAllRtp -> `if on modify => disable`
   */

  std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>,
             std::vector<uint8_t>>
  GetAllRegisters();

  void UpdateAllRegisters(
      std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>>);

  /* Update Single Device Registers */
  template <class T>
  void UpdateRegisters(std::string target, std::vector<uint8_t> val) {  // drv_x, drv_y, drv_z, adxl
    if (target == "drv_x") {
      ChangeDrvCh('x');
      drv_x_->UpdateAllReg(val);
    } else if (target == "drv_y") {
      ChangeDrvCh('y');
      drv_y_->UpdateAllReg(val);
    } else if (target == "drv_z") {
      ChangeDrvCh('z');
      drv_z_->UpdateAllReg(val);
    } else if (target == "adxl") {
      adxl_->UpdateAllReg(val);
    } else {
      logunit_->LogToDefault(loglevel::err, "MainController UpdateRegisters failed, target: {} mismatch\n", target);
    }
  }

 private:
  std::shared_ptr<LogUnit> logunit_{nullptr};
  std::shared_ptr<Tca9548a> tca_{nullptr};
  char tca_ch_{'x'};
};

bool Controller::new_acc_data_ = false;

}  // namespace lra::controller

#endif