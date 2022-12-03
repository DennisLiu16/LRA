#include <device/drv2605l/drv2605l.h>

namespace lra::device {
Drv2605l::Drv2605l(const I2cDeviceInfo& info, std::string name) {
  info_ = info;
  name_ = name;
  logunit_ = lra::log_util::LogUnit::CreateLogUnit(name_);

  // you can add register check here
}

bool Drv2605l::Init(I2cAdapter_S init_s) {
  init_s.dev_info_ = &info_;

  return adapter_.Init(init_s);
}

ssize_t Drv2605l::Write(const uint8_t addr, const uint8_t* val, const uint32_t len) {
  // val will never be negative number
  // Write(const uint8_t&, const uint8_t*, uint32_t len)
  return adapter_.Write(addr, val, len);
}

ssize_t Drv2605l::Read(const uint8_t addr, uint8_t* val, const uint16_t len) { return adapter_.Read(addr, val, len); }

std::vector<uint8_t> Drv2605l::Read(const uint8_t addr, const uint16_t len) {
  std::vector<uint8_t> vec(len);
  Read(addr, vec.data(), len);
  return vec;
}

int16_t Drv2605l::Read(const uint8_t addr) {
  // you can use other integral type here to store read 1 byte value
  uint8_t val = 0;
  return (adapter_.Read(addr, val) == sizeof(uint8_t)) ? val : std::to_underlying(Errors::kRead);
}

// functions
std::vector<uint8_t> Drv2605l::GetAllReg() {
  const int r_reg_len = LRA_PERIOD.addr_ + 1;
  return Read(0x00, r_reg_len);
}

void Drv2605l::UpdateAllReg(std::vector<uint8_t> v) {
  const int w_reg_len = LRA_PERIOD.addr_;  // 0x00 is RO

  if (v.size() == w_reg_len) {
    if (v[0] >> 7 == 1) {  // reset cmd
      logunit_->LogToDefault(loglevel::err, "drv: {} UpdateAllReg may failed: reset bit in Mode is set\n", name_);
    }
    Write(MODE.addr_, v.data(), w_reg_len);
  } else {
    logunit_->LogToDefault(loglevel::err, "drv: {} UpdateAllReg failed: length: {} mismatch {}\n", name_, v.size(),
                           w_reg_len);
  }
}

void Drv2605l::SetToLraDefault() {
  /* reset */
  Write(MODE, 0x01 << 7);
  usleep(1000000);

  /* necessary */
  Write(MODE, 0x01 << 6 | 0x05);  // RTP mode, standby
  Write(RTP_INPUT, 0x00);
  Write(LIBRARY_SELECTION, 0x06);  // LRA lib
  Write(RATED_VOLTAGE, 0x3E);      // voltage at steady state
  Write(FEEDBACK_CONTROL,
        0x01 << 7 | 0x03 << 4 | 0x01 << 2 | 0x02);  // LRA mode
  Write(OD_CLAMP, 0xFF);
  Write(CONTROL1, 0x01 << 7 | 0x00 << 5 | 0x13);                               // boost on
  Write(CONTROL2, 0x00 << 7 | 0x01 << 6 | 0x01 << 4 | 0x02 << 2 | 0x02);       // un bidir
  Write(CONTROL3, 0x01 << 6 | 0x01 << 5 | 0x00 << 4 | 0x01 << 3 | 0x01 << 2);  // RTP unsigned ??
  Write(CONTROL4, 0x03 << 4);                                                  // autocalibration 1000~1200 ms
  Write(CONTROL5, 0x02 << 6 | 0x00 << 5 | 0x01 << 4);

  /* TODO: custom */

  logunit_->LogToDefault(loglevel::info, "drv: {} set to lra defaults\n", name_);
}

Drv2605lInfo Drv2605l::GetCalibrationInfo() {
  Drv2605lInfo info;
  int val = Read(STATUS);
  if (val >= 0) {
    info.device_id_ = [](uint8_t id) -> std::string {
      if (id == 4)
        return "DRV2604";
      else if (id == 3)
        return "DRV2605";
      else if (id == 6)
        return "DRV2604L";
      else if (id == 7)
        return "DRV2605L";
      else
        return "unknown";
    }(val >> 5);

    info.over_current_detect_ = val & (0x1 << 1);
    info.over_temp_detect_ = val & 0x01;
    info.diag_result_ = val & (0x01 << 3); 
  }

  val = Read(A_CAL_COMP);
  if (val >= 0) info.compensation_coeff_ = 1 + val / 255;

  val = Read(A_CAL_BEMF);
  if (val >= 0) {
    auto fc_val = Read(FEEDBACK_CONTROL);
    if (fc_val >= 0)
      info.back_emf_result_ = val / 255 * 1.22 / [](uint8_t val) -> float {
        if ((val & 0x03) == 0x00)
          return (val & (0x01 << 7)) ? 3.75 : 0.255;  // lra/erm
        else if ((val & 0x03) == 0x01)
          return (val & (0x01 << 7)) ? 7.5 : 0.7875;
        else if ((val & 0x03) == 0x02)
          return (val & (0x01 << 7)) ? 15 : 1.365;
        else
          return (val & (0x01 << 7)) ? 22.5 : 3.0;  // 0x03
      }(fc_val);
  }

  info.lra_freq_ = GetHz();

  val = Read(VBAT);
  if (val >= 0) {
    info.vbat_ = val * 5.6 / 255;
  }

  return info;
}

Drv2605lInfo Drv2605l::RunAutoCalibration() {
  auto mode = Read(MODE) & 0x07;  // get current mode, with mask (00000111)
  Write(MODE, 0x01 << 6 | 0x07);  // calibration mode, ready
  Run(true);                      // take 1 ~ 1.2 sec
  usleep(1200000);                // sleep 1.2 sec
  Run(false);                     // fix internal state run_
  Write(MODE, 0x01 << 6 | mode);  // resume previous mode, standby
  return GetCalibrationInfo();
}

Drv2605lRtInfo Drv2605l::GetRt() {
  Drv2605lRtInfo tmp;
  tmp.rtp_ = Read(RTP_INPUT);
  tmp.lra_freq_ = GetHz();
  return tmp;
}

float Drv2605l::GetHz() {
  auto val = Read(LRA_PERIOD);
  return 1.0 / (val * 98.46 * 1e-6);
}

void Drv2605l::Ready(bool ready) {
  auto val = Read(MODE);
  if (val < 0) {
    logunit_->LogToDefault(loglevel::err, "drv: {}, execute Run() failed, due to read register MODE failed\n", name_);
    return;
  } else {
    constexpr uint8_t sixth_bit_inverse_mask = 0b10111111;
    if (ready)
      Write(MODE, (val & sixth_bit_inverse_mask) | 0x00 << 6);
    else
      Write(MODE, (val & sixth_bit_inverse_mask) | 0x01 << 6);
  }
}

void Drv2605l::Run(bool run) {
  if(run_ != run) {
    Ready(run);
    Write(GO, (uint8_t)run);
    run_ = run;
  }
}

void Drv2605l::UpdateRTP(uint8_t cmd) { Write(RTP_INPUT, cmd); }

}  // namespace lra::device