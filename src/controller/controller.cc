#include <controller/controller.h>
#include <spdlog/fmt/chrono.h>

/**
 * @brief : 流程如下
 * # 變更、 讀取 register 都不在 real time plot 中 => 理論不會衝突 (broadcast 除外 -> stop broadcast or receive new cmd
 * when sending UpdateRequire) => onModify of Controller, no cmd should be execute
 *
 * 1. create a unique_ptr of Controller
 * 2. Init(), init bus and devices
 * 3. RunCalibration()
 * 4. Run()
 * 5. PauseDriving() and onModify [ignore new cmd and Acc 的 GetAllReg 和 SetAllReg 是否 standby 保護的 ] -> onModify
 * 關成 acc standby -> 有遇到 autocalibration 在關掉線程
 * 6. UpdateAllRegisters... or Autocalibration
 */

namespace lra::controller {

Controller::Controller() {
  start_time_ = std::chrono::system_clock::now();

  // init bus
  bool rtn = i2c_.Init("/dev/i2c-1");
  logunit_ = LogUnit::CreateLogUnit("MainController");

  logunit_->LogToDefault(loglevel::info, "MainController try to create, start time: {:%Y-%m-%d %H:%M:%S}\n", start_time_);

  if (rtn) {
    logunit_->LogToDefault(loglevel::info, "MainController I2C bus init successfully, speed: {}\n", i2c_.speed_);

  } else {
    logunit_->LogToDefault(loglevel::critical, "MainController I2C bus init failed\n");
  }
}

void Controller::Init() {
  CancelMeasureTask();
  logunit_->LogToDefault(loglevel::info, "MainController on Init()\n");

  /* I2C adapter init */
  i2c_adapter_s_.bus_ = std::make_shared<I2c>(i2c_);
  i2c_adapter_s_.delay_ = 0;
  i2c_adapter_s_.method_ = I2c::I2cMethod::kSmbus;
  i2c_adapter_s_.name_ = "i2c_adapter";

  /* I2c device info settings */
  info_tca_.addr_ = 0x70;
  info_tca_.flags_ = 0;
  info_tca_.iaddr_bytes_ = 0;
  info_tca_.page_bytes_ = 16;
  info_tca_.tenbit_ = false;

  info_drv_.addr_ = 0x5a;
  info_drv_.flags_ = 0;
  info_drv_.iaddr_bytes_ = 1;
  info_drv_.page_bytes_ = 32;
  info_drv_.tenbit_ = false;

  /* SPI info init */
  info_adxl_.channel_ = 0;
  info_adxl_.mode_ = 0;
  info_adxl_.speed_ = 10000000;

  /* I2c device creates new shared pointer */
  tca_ = std::make_shared<Tca9548a>(info_tca_);
  drv_x_ = std::make_shared<Drv2605l>(info_drv_, "drv_x");
  drv_y_ = std::make_shared<Drv2605l>(info_drv_, "drv_y");
  drv_z_ = std::make_shared<Drv2605l>(info_drv_, "drv_z");

  /* adxl355 creates shared ptr */
  adxl_ = std::make_shared<Adxl355>();

  /* devices init */
  tca_->Init(i2c_adapter_s_);
  drv_x_->Init(i2c_adapter_s_);
  drv_y_->Init(i2c_adapter_s_);
  drv_z_->Init(i2c_adapter_s_);
  adxl_->Init(info_adxl_, "acc1");

  /* Can be deleted custom */
  drv_x_->SetToLraDefault();
  drv_y_->SetToLraDefault();
  drv_z_->SetToLraDefault();

  /* IT pin settings */
  const int interrupt_pin = 6;
  pinMode(interrupt_pin, INPUT);
  pullUpDnControl(interrupt_pin, PUD_DOWN);
  wiringPiISR(interrupt_pin, INT_EDGE_RISING, ItCallback);

  /* Start measure task if threading not exist */
  StartMeasureTask();
}

void Controller::UpdateAllRtp(std::tuple<uint8_t, uint8_t, uint8_t> val) {
  auto [val_x, val_y, val_z] = val;

  ChangeDrvCh('x');
  drv_x_->UpdateRTP(val_x);

  ChangeDrvCh('y');
  drv_y_->UpdateRTP(val_y);

  ChangeDrvCh('z');
  drv_z_->UpdateRTP(val_z);
}

void Controller::UpdateRtp(uint8_t val, char axis) {
  if (axis == 'x' || axis == 'y' || axis == 'z') {
    ChangeDrvCh(axis);
    switch (axis) {
      case 'x':
        drv_x_->UpdateRTP(val);
        break;
      case 'y':
        drv_y_->UpdateRTP(val);
        break;
      case 'z':
        drv_z_->UpdateRTP(val);
        break;
    }
  } else {
    logunit_->LogToDefault(loglevel::err, "MainController UpdateRtp failed: axis '{}' mismatch\n", axis);
  }
}

void Controller::ChangeDrvCh(char axis) {
  switch (axis) {
    case 'x':
      tca_->Write(tca_->CONTROL, drv_x_ch_);
      tca_ch_ = 'x';
      break;
    case 'y':
      tca_->Write(tca_->CONTROL, drv_y_ch_);
      tca_ch_ = 'y';
      break;
    case 'z':
      tca_->Write(tca_->CONTROL, drv_z_ch_);
      tca_ch_ = 'z';
      break;
    default:
      break;
  }
}

void Controller::Run() {
  ChangeDrvCh('x');
  drv_x_->Run(true);
  ChangeDrvCh('y');
  drv_y_->Run(true);
  ChangeDrvCh('z');
  drv_z_->Run(true);
}

/* Stop drv driving, should be called when disconnect of websocekt or pause being called by user */
void Controller::PauseDriving() {
  ChangeDrvCh('x');
  drv_x_->Run(false);
  ChangeDrvCh('y');
  drv_y_->Run(false);
  ChangeDrvCh('z');
  drv_z_->Run(false);
}

std::tuple<Drv2605lInfo, Drv2605lInfo, Drv2605lInfo, Adxl355::Acc3> Controller::RunCalibration() {
  /* three axis Drv2605 */
  auto cal_info_x = drv_x_->RunAutoCalibration();
  auto cal_info_y = drv_y_->RunAutoCalibration();
  auto cal_info_z = drv_z_->RunAutoCalibration();

  /* acc bias correction */
  constexpr int data_num = 5000;
  std::vector<Adxl355::Acc3> v;
  v.reserve(data_num);

  for (int n = data_num - v.size(); n > 0; n = data_num - v.size()) {  // get data from dq
    auto v_tmp = adxl_->AccPopFrontN(n);
    v.insert(v.end(), v_tmp.begin(), v_tmp.end());
  }

  /* calculate average */
  Adxl355::Acc3 acc_avg;
  for (auto& ele : v) {  // TODO: struct opertor
    acc_avg.data.x += ele.data.x;
    acc_avg.data.y += ele.data.y;
    acc_avg.data.z += ele.data.z;
  }

  acc_avg.data.x /= v.size();
  acc_avg.data.y /= v.size();
  acc_avg.data.z /= v.size();

  adxl_->SetOffSet(acc_avg);

  logunit_->LogToDefault(loglevel::info, "MainController finished calibration\n");
  return std::make_tuple(cal_info_x, cal_info_y, cal_info_z, acc_avg);
}

void Controller::AccMeasureTask() {
  adxl355_measure_thread_exit_ = false;

  while (!adxl355_measure_thread_exit_) {
    // if (adxl_->standby_) { 如果是
    if (Controller::new_acc_data_) {  // 當 standby mode 時不會觸發，所以若不是校正模式不用重開
      Adxl355::Acc3 data = adxl_->GetAcc();
      data.time = (std::chrono::system_clock::now() - start_time_).count();
      adxl_->AccPushBack(data);

      if (adxl_->GetDataDequeSize() > max_number_in_deque_) {  // protect memory overflow
        adxl_->AccPopFront();
      }

      Controller::new_acc_data_ = false;
    } else {
      // XXX: may be a problem ?
      std::this_thread::yield();
    }
  }
  return;
}

void Controller::ItCallback() { Controller::new_acc_data_ = true; }

/* Should and only be called before destroy MainController */
void Controller::CancelMeasureTask() {
  if (adxl355_measure_t_.get_id() == std::thread::id()) {  // not a running thread
    return;
  } else {
    adxl355_measure_thread_exit_ = true;

    try {  // wait for the thread leaving
      adxl355_measure_t_.join();
      logunit_->LogToDefault(loglevel::err, "MainController CancelMeasureTask successfully\n");
    } catch (const std::exception& e) {
      logunit_->LogToDefault(loglevel::err, "MainController CancelMeasureTask failed\n, detail: {}", e.what());
    }
  }
}

void Controller::StartMeasureTask() {
  if (adxl355_measure_t_.get_id() == std::thread::id()) {  // a null thread
    std::thread adxl355_measure_t_(&Controller::AccMeasureTask, this);
    logunit_->LogToDefault(loglevel::info, "MainController StartMeasureTask successfully\n");
  } else {
    logunit_->LogToDefault(loglevel::warn, "MainController StartMeasureTask failed, thread existed\n");
  }
  return;
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>>
Controller::GetAllRegisters() {
  ChangeDrvCh('x');
  auto drv_x_v = drv_x_->GetAllReg();
  ChangeDrvCh('y');
  auto drv_y_v = drv_y_->GetAllReg();
  ChangeDrvCh('z');
  auto drv_z_v = drv_z_->GetAllReg();

  auto [adxl_v_ro, adxl_v_rw] = adxl_->GetAllReg();

  return std::make_tuple(drv_x_v, drv_y_v, drv_z_v, adxl_v_ro, adxl_v_rw);
}

void Controller::UpdateAllRegisters(
    std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>> tuple) {
  auto [v_x, v_y, v_z, v_acc_rw] = tuple;

  ChangeDrvCh('x');
  drv_x_->UpdateAllReg(v_x);
  ChangeDrvCh('y');
  drv_y_->UpdateAllReg(v_y);
  ChangeDrvCh('z');
  drv_z_->UpdateAllReg(v_z);

  adxl_->UpdateAllReg(v_acc_rw);

  logunit_->LogToDefault(loglevel::info, "MainController UpdateAllRegisters successfully\n");
}

Controller::~Controller() { CancelMeasureTask(); }

}  // namespace lra::controller
