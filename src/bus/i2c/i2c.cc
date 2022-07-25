#include <bus/i2c/i2c.h>
#include <bus/i2c/i2c_sys.h>

namespace lra::bus::i2c {
// CRTP
bool InitImpl(const I2cInit_S& init_s) {
  // TODO
}

bool I2c::InitImpl(const char* bus_name) {
  // init logger
  logunit_ = ::lra::log_util::LogUnit::CreateLogUnit(bus_name);
  // open i2c channel
  int fd = sys::I2cOpen(bus_name);
  if (fd < 0) {  // failed
    logunit_->Log(spdlog::level::err, "I2C bus: {} open failed\n", bus_name);
    logunit_ = nullptr;
  } else {  // successful
    bus_name_ = bus_name;
    fd_ = fd;
  }

  // get speed
  std::string current_version = ::lra::terminal_util::getKernelVersion();
  if (IsPiDistribution(current_version)) {
    getPiI2cSpeed();
  } else {
    // TODO: Warning you need to implement get I2C speed in kernel verision: ...
    speed_ = 100000;
  }
}

// WriteImpl Template Specialize
template <> ssize_t I2c::WriteImpl<>;

ssize_t I2c::WriteImpl(const auto &reg, uint8_t val) {

}

// Personal functions
bool I2c::IsPiDistribution(std::string_view current_version) {
  return (current_version.find("raspi") != std::string::npos);
}

uint32_t I2c::getPiI2cSpeed() {
  std::string file_path = "/sys/bus/i2c/devices/i2c-";
  std::string_view bus_name_s(bus_name_);
  file_path += bus_name_s.back();
  file_path += "/of_node/clock-frequency";
  std::string cmd = "xxd -ps ";
  std::string result = lra::terminal_util::Execute(cmd.append(file_path).c_str());
  if (result.find("xxd") != std::string::npos) {  // cmd failed
    logunit_->Log(spdlog::level::warn,
                  "I2C bus: {} get speed failed\nError code: {}\nAssume I2c default speed 100k bits/sec\n", bus_name_,
                  result);
    speed_ = 100000;
  } else {  // change to decimal
    speed_ = std::stoul(result, nullptr, 16);
    logunit_->Log(spdlog::level::info, "I2C bus: {} get clock frequency: {} bits/sec\n", bus_name_, speed_);
  }
}

I2c::~I2c() { sys::I2cClose(fd_); }
}  // namespace lra::bus::i2c