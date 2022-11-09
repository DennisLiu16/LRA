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
  std::vector<uint8_t> v_tmp;
  v_tmp.push_back(addr << 1 | 0x00);
  v_tmp.insert(v_tmp.end(), val, val + len);

  rw_mutex_.lock();
  int num = wiringPiSPIDataRW(init_.channel_, v_tmp.data(), len + 1);
  rw_mutex_.unlock();

  if (num - 1 != len)
    logunit_->LogToDefault(loglevel::err, "adxl: {} write failed: len mismatch, rtn: {} != len: {}\n", name_, num - 1,
                           len);
  return num - 1;
}

std::vector<uint8_t> Adxl355::Read(const uint8_t addr, const uint16_t len) {
  // ref: https://stackoverflow.com/questions/15004517/moving-elements-from-stdvector-to-another-one
  std::vector<uint8_t> v_tmp, v_rtn;
  v_tmp.resize(len + 1);
  v_tmp[0] = addr << 1 | 0x01;  // 0 for write, 1 for read

  rw_mutex_.lock();
  int num = wiringPiSPIDataRW(init_.channel_, v_tmp.data(), len + 1);
  rw_mutex_.unlock();

  if (num - 1 != len) {
    logunit_->LogToDefault(loglevel::err, "adxl: {} read failed: len mismatch, rtn: {} != len: {}\n", name_, num - 1,
                           len);
  } else {
    v_rtn.insert(v_rtn.end(), std::make_move_iterator(v_tmp.begin() + 1), std::make_move_iterator(v_tmp.end()));
  }
  return v_rtn;
}

void Adxl355::SetToDefault() {
  /* reset device, standby mode; it's ok to write reset in measure mode */
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
  // bool tmp = standby_; // only write need to protect
  // SetStandBy(true);

  // avoid FIFO 0x11
  constexpr int ro_reg_num = 0x10 + 1;
  constexpr int rw_reg_num = 18;  // FIXME: RESET is WO register?

  auto v1 = Read(DEVID_AD.addr_, ro_reg_num);    // RO
  auto v2 = Read(OFFSET_X_H.addr_, rw_reg_num);  // RW, including reset

  // SetStandBy(tmp);

  return std::make_tuple(v1, v2);
}

/* FIXME: not thread safe, need a mutex ? */
void Adxl355::UpdateAllReg(const std::vector<uint8_t> v) {
  bool tmp = standby_;
  SetStandBy(true);

  constexpr int range_idx = 14;
  constexpr int rw_reg_num = 18;

  if (v.size() == rw_reg_num) {
    // update useful state
    int tmp_range = v[range_idx] & ((1 << 2) - 1);
    if (tmp_range == 0x00) {
      tmp_range = 0b10;  // user input protect -> check range not 0x00
      logunit_->LogToDefault(loglevel::warn,
                             "adxl: {} UpdateAllReg range setting wrong, can't be 0x00 @ RANGE register\n", name_);
    }
    range_ = tmp_range;  // FIXME: when update only range registers -> cache failed -> GetCacheRange failed too
    Write(OFFSET_X_H.addr_, v.data(), v.size());
  } else {
    logunit_->LogToDefault(loglevel::err, "adxl: {} UpdateAllReg failed: length mismatch, v.size(): {} should be {}\n",
                           name_, v.size(), rw_reg_num);
  }

  // FIXME: This can protect mode, but will modify user input (mode)
  SetStandBy(tmp);
}

/* FIXME: not thread safe, need a mutex ? */
void Adxl355::SetOffSet(Adxl355::Acc3 acc) {
  bool tmp = standby_;
  SetStandBy(true);

  constexpr int offset_reg_num = 6;
  constexpr int offset_adc_num = 65536;  // 2^16
  const float dAccRange = GetCacheRange();

  int16_t intX = round(acc.data.x / dAccRange * offset_adc_num);
  int16_t intY = round(acc.data.y / dAccRange * offset_adc_num);
  int16_t intZ = round(acc.data.z / dAccRange * offset_adc_num);

  /* check not overflow, offset_adc_num / 2 */
  if (abs(intX) > (offset_adc_num >> 1) || abs(intY) > (offset_adc_num >> 1) || abs(intZ) > (offset_adc_num >> 1)) {
    logunit_->LogToDefault(loglevel::err,
                           "adxl: {} SetOffSet failed: data OverRange.\n x: {}, y: {}, z:{}\nChanges aborted\n", name_,
                           acc.data.x, acc.data.y, acc.data.z);
    return;
  }

  uint8_t new_offset[] = {
      static_cast<uint8_t>(intX >> 8), static_cast<uint8_t>(intX),      static_cast<uint8_t>(intY >> 8),
      static_cast<uint8_t>(intY),      static_cast<uint8_t>(intZ >> 8), static_cast<uint8_t>(intZ),
  };

  Write(OFFSET_X_H.addr_, new_offset, offset_reg_num);

  SetStandBy(
      tmp);  // FIXME: if other thread call some functions use SetStandby, this may break the standby if tmp is false.
}

Adxl355::Acc3 Adxl355::GetOffSet() {
  constexpr int offset_len = 6;
  constexpr int offset_adc_num = 65536;  // 2^16
  const float dAccRange = GetCacheRange();
  auto v = Read(OFFSET_X_H.addr_, offset_len);

  uint16_t uintX = v[0] << 8 | v[1];
  uint16_t uintY = v[2] << 8 | v[3];
  uint16_t uintZ = v[4] << 8 | v[5];

  int16_t intX = uintX, intY = uintY, intZ = uintZ;

  Acc3 offset;

  offset.data.x = (double)intX * (1.0 / offset_adc_num) * dAccRange;
  offset.data.y = (double)intY * (1.0 / offset_adc_num) * dAccRange;
  offset.data.z = (double)intZ * (1.0 / offset_adc_num) * dAccRange;

  return offset;
}

/* Important: You should confirm standby_ is false in the measuring thread */
/* Assign time by your self */
Adxl355::Acc3 Adxl355::GetAcc() {
  constexpr int data_len = 9;
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
    constexpr uint32_t mask_20 = (1 << 20) - 1;
    int32_t intX = ((uintX & (1 << 19)) != 0) ? (uintX | ~mask_20) : uintX;
    int32_t intY = ((uintY & (1 << 19)) != 0) ? (uintY | ~mask_20) : uintY;
    int32_t intZ = ((uintZ & (1 << 19)) != 0) ? (uintZ | ~mask_20) : uintZ;

    // int to float
    constexpr uint32_t acc_adc_num = 1048576;  // (2^20)
    const float dAccRange = GetCacheRange();

    tmp.data.x = ((double)intX) * (1.0 / acc_adc_num) * dAccRange;
    tmp.data.y = ((double)intY) * (1.0 / acc_adc_num) * dAccRange;
    tmp.data.z = ((double)intZ) * (1.0 / acc_adc_num) * dAccRange;
  }
  return tmp;
}

// inverse

float Adxl355::GetCacheRange() {
  float dAccRange;

  if (range_ == 0x01)
    dAccRange = dRange_2g;
  else if (range_ == 0x10)
    dAccRange = dRange_4g;
  else if (range_ == 0x11)
    dAccRange = dRange_8g;
  else {
    logunit_->LogToDefault(loglevel::err, "adxl: {} GetCacheRange: range is 0x00 .Not allows!\n", name_, range_);
    dAccRange = dRange_4g;
  }

  return dAccRange;
}

/* should check deque's size by yourself, not safe so you need to avoid to use this function */
Adxl355::Acc3 Adxl355::AccPopFront() {
  std::lock_guard<std::mutex> lock(dq_mutex_);
  auto tmp = data_.front();
  data_.pop_front();
  return tmp;
}

std::vector<Adxl355::Acc3> Adxl355::AccPopAll() { return AccPopFrontN(data_.size()); }

std::vector<Adxl355::Acc3> Adxl355::AccPopFrontN(size_t n) {
  std::vector<Acc3> tmp;
  if (n <= 0) return tmp;

  std::lock_guard<std::mutex> lock(dq_mutex_);

  if (data_.size() < n) {
    n = data_.size();
  }

  tmp.reserve(n);

  while (n--) {
    tmp.push_back(data_.front());
    data_.pop_front();
  }

  return tmp;
}

void Adxl355::AccPushBack(Acc3 acc_data) {
  std::lock_guard<std::mutex> lock(dq_mutex_);
  data_.push_back(acc_data);
}

}  // namespace lra::device