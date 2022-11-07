#include <device/adxl355/adxl355.h>

#include <tuple>

namespace lra::device {
std::string Adxl355::CheckDeviceReg() {  // device 繼承
  constexpr auto regsinfo = lra::memory::registers::_getRegistersInfo(regs_);
  constexpr auto total_bytes = lra::memory::registers::_SumArray(regsinfo.nbytes_);
  constexpr auto total_regs = lra::memory::registers::_SumArray(regsinfo.nregs_);
  constexpr auto overlay_len = lra::memory::registers::_getOverlayLen<total_bytes>(regs_);
  constexpr auto overlay_regs = lra::memory::registers::_getOverlayReg<total_bytes, overlay_len>(regs_);

  std::string s;

  s += spdlog::fmt_lib::format("total bytes: {}\n", total_bytes);
  s += spdlog::fmt_lib::format("total registers: {}\n", total_regs);
  s += spdlog::fmt_lib::format("overlay len: {}\n\n", overlay_len);

  // make a constexpr for (integral only) here
  lra::memory::registers::constexpr_for<0, overlay_regs.size(), 1>([&overlay_regs, &s](auto i) {  // add visit func
    uint64_t addr1 = lra::memory::registers::_VisitAddr(*overlay_regs.at(i).reg1_);
    uint64_t addr2 = lra::memory::registers::_VisitAddr(*overlay_regs.at(i).reg2_);
    constexpr uint8_t v_idx1 = overlay_regs.at(i).reg1_->index();
    constexpr uint8_t v_idx2 = overlay_regs.at(i).reg2_->index();
    std::string name1 = boost::core::demangle(typeid(std::variant_alternative_t<v_idx1, Register_T>).name());
    std::string name2 = boost::core::demangle(typeid(std::variant_alternative_t<v_idx2, Register_T>).name());
    s += spdlog::fmt_lib::format("@: {0:#0x}\n {2}({1:#0x}), {4}({3:#0x})\n", overlay_regs.at(i).overlap_addr_, addr1,
                                 name1, addr2, name2);
  });
  // spdlog::fmt_lib::print(spdlog::fmt_lib::runtime(s)); fmt problem solve
  return s;
}

void Adxl355::Init(SpiInit_s s_init, std::string name) {
  wiringPiSPISetupMode(s_init.channel_, s_init.speed_, s_init.mode_);
  logunit_ = lra::log_util::LogUnit::CreateLogUnit(name);

  if (auto fd = wiringPiSPIGetFd(s_init.channel_); fd > 0) {
    name_ = name;
    init_ = s_init;
    logunit_->LogToDefault(loglevel::info, "adxl: {} init succesfully, fd: {}\n, ", name_, fd);
    SetToDefault();
    standby_ = true;
  } else {
    logunit_->LogToDefault(loglevel::err, "adxl: {} init failed\n", name);
  }

  // should call adapter_.Init();
}

ssize_t Adxl355::Write(const uint8_t addr, const uint8_t* val, const uint16_t len) {
  std::lock_guard<std::mutex> lock(rw_mutex_);
  std::vector<uint8_t> v_tmp;
  v_tmp.push_back(addr << 1 | 0x00);
  v_tmp.insert(v_tmp.end(), val, val + len);

  int num = wiringPiSPIDataRW(init_.channel_, v_tmp.data(), len + 1);
  if (num - 1 != len)
    logunit_->LogToDefault(loglevel::err, "adxl: {} write failed: len mismatch, rtn: {} != len: {}\n", name_, num - 1,
                           len);
  return num - 1;
}

std::vector<uint8_t> Adxl355::Read(const uint8_t addr, const uint16_t len) {
  // ref: https://stackoverflow.com/questions/15004517/moving-elements-from-stdvector-to-another-one
  std::lock_guard<std::mutex> lock(rw_mutex_);
  std::vector<uint8_t> v_tmp, v_rtn;
  v_tmp.resize(len + 1);
  v_tmp[0] = addr << 1 | 0x01;  // 0 for write, 1 for read
  int num = wiringPiSPIDataRW(init_.channel_, v_tmp.data(), len + 1);
  if (num - 1 != len) {
    logunit_->LogToDefault(loglevel::err, "adxl: {} read failed: len mismatch, rtn: {} != len: {}\n", name_, num - 1,
                           len);
  } else {
    v_rtn.insert(v_rtn.end(), std::make_move_iterator(v_tmp.begin() + 1), std::make_move_iterator(v_tmp.end()));
  }
  return v_rtn;
}

void Adxl355::SetToDefault() {
  /* reset device, standby mode */
  uint8_t val = 0x52;
  Write(Reset.addr_, &val, 1);

  /* set range to 4g */
  val = 0x01 << 6 | 0x10;  // INT active high and 4g
  Write(Range.addr_, &val, 1);

  /* INT_MAP to INT1*/
  val = 0x01 << 3 | 0x01;
  Write(INT_MAP.addr_, &val, 1);

  /* set sampling rate */
  val = (0b100 << 4) | 0x00;  // 10 Hz hpf, 4000 Hz sampling rate, 1000 Hz lpf
  Write(Filter.addr_, &val, 1);
}

void Adxl355::SetStandBy(bool standby) {
  if (standby_ != standby) {
    auto val = Read(POWER_CTL.addr_, 1);
    uint8_t v = (val[0] & 0x0) | standby;
    auto rtn = Write(POWER_CTL.addr_, &v, 1);

    if (rtn == 1) standby_ = standby;
  }
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> Adxl355::GetAllReg() {
  bool tmp = standby_;
  SetStandBy(true);

  // avoid FIFO 0x11
  const int ro_reg_num = 11;
  const int rw_reg_num = 18;

  auto v1 = Read(DEVID_AD.addr_, ro_reg_num);    // RO
  auto v2 = Read(OFFSET_X_H.addr_, rw_reg_num);  // RW, including reset

  SetStandBy(tmp);

  return std::make_tuple(v1, v2);
}

void Adxl355::SetAllReg(const std::vector<uint8_t> v) {
  bool tmp = standby_;
  SetStandBy(true);

  const int range_idx = 14;
  const int rw_reg_num = 18;

  if (v.size() == rw_reg_num) {
    // update useful state
    int tmp_range = v[range_idx] & ((1 << 2) - 1);
    if (tmp_range == 0) tmp_range = 0b10;      // user input protect
    range_ = tmp_range;
    Write(OFFSET_X_H.addr_, v.data(), 18);
  }

  // FIXME: This can protect mode, but will modify user input (mode)
  SetStandBy(tmp);
}

/* Important: You should confirm standby_ is false in the measuring thread */
/* Assign time by your self */
Adxl355::Acc3 Adxl355::GetAcc() {
  const int data_len = 9;
  auto v = Read(XDATA3.addr_, data_len);
  return ParseDigitalAcc(v);
}

Adxl355::Acc3 Adxl355::ParseDigitalAcc(std::vector<uint8_t> v) {
  Acc3 tmp;

  if (v.size() != 9) {  // 3 * 3 {
    logunit_->LogToDefault(loglevel::err, "adxl: {} parse acc data failed: length mismatch, v.size(): {}\n", name_,
                           v.size());

  } else {
    uint32_t uintX = (v[0] << 12) | (v[1] << 4) | (v[2] >> 4);
    uint32_t uintY = (v[3] << 12) | (v[4] << 4) | (v[5] >> 4);
    uint32_t uintZ = (v[6] << 12) | (v[7] << 4) | (v[8] >> 4);

    // do two component according to 19th bit, if 1 -> convert, 0 -> same
    const uint32_t mask_20 = (1 << 20) - 1;
    int32_t intX = ((uintX & (1 << 19)) != 0) ? (uintX | ~mask_20) : uintX;
    int32_t intY = ((uintY & (1 << 19)) != 0) ? (uintY | ~mask_20) : uintY;
    int32_t intZ = ((uintZ & (1 << 19)) != 0) ? (uintZ | ~mask_20) : uintZ;

    // int to float
    const uint32_t acc_adc_num = 524288;  // (2^20/2)
    float dAccRange;

    if (range_ == 0x01)
      dAccRange = dRange_2g;
    else if (range_ == 0x10)
      dAccRange = dRange_4g;
    else if (range_ == 0x11)
      dAccRange = dRange_8g;
    else
      logunit_->LogToDefault(loglevel::err, "adxl: {} parse digital acc failed: range == {} \n", name_, range_);

    tmp.data.x = ((double)intX) * (1.0 / acc_adc_num) * dAccRange;
    tmp.data.y = ((double)intY) * (1.0 / acc_adc_num) * dAccRange;
    tmp.data.z = ((double)intZ) * (1.0 / acc_adc_num) * dAccRange;
  }
  return tmp;
}

// inverse

Adxl355::Acc3 Adxl355::AccPopFront() {
  std::lock_guard<std::mutex> lock(dq_mutex_);
  auto tmp = data_.front();
  data_.pop_front();
  return tmp;
}

void Adxl355::AccPushBack(Acc3 acc_data) {
  std::lock_guard<std::mutex> lock(dq_mutex_);
  data_.push_back(acc_data);
}

}  // namespace lra::device