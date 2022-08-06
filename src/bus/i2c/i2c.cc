#include <bus/i2c/i2c.h>

namespace lra::bus {  // no logunit should be used

// CRTP Impl
bool I2c::InitImpl(const I2cInit_S& init_s) {
  if (InitImpl(init_s.name_)) {
    // TODO: Init other param
    return true;
  }
  return false;
}

bool I2c::InitImpl(const char* bus_name) {
  int fd = I2cOpen(bus_name);
  if (fd < 0) {  // failed
    return false;
  } else {  // successful
    I2cResetThisBus();
    name_ = bus_name;
    fd_ = fd;
  }

  // update func_
  I2cUpdateThisBusFunc();

  // get speed
  std::string current_version = ::lra::terminal_util::getKernelVersion();
  if (IsPiDistribution(current_version)) {
    UpdateI2cSpeedOnPi();
  } else {  // you can implement other distribution version
    speed_ = 0;
  }
  return true;
}

template <I2c::I2cMethod method>
ssize_t I2c::WriteImpl(const void* data) {
  return WriteMultiImpl<method>(data);
}

template <I2c::I2cMethod method>
ssize_t I2c::WriteMultiImpl(const void* data) {
  if constexpr (I2cMethod::kPlain == method) {  // I2C plain device, prefer

    auto typed_data = static_cast<const i2c_rdwr_ioctl_data*>(data);
    return PlainRW(typed_data);

  } else if constexpr (I2cMethod::kSmbus == method) {  // I2C SMBUS

    auto typed_data = static_cast<const i2c_rdwr_smbus_data*>(data);
    return SmbusRW<false>(typed_data);  // false -> write cmd
  }
}

template <I2c::I2cMethod method>
ssize_t I2c::ReadImpl(void* data) {
  return ReadMultiImpl<method>(data);
}

template <I2c::I2cMethod method>
ssize_t I2c::ReadMultiImpl(void* data) {
  if constexpr (I2cMethod::kPlain == method) {  // I2C plain device, prefer

    auto typed_data = static_cast<i2c_rdwr_ioctl_data*>(data);
    return PlainRW(typed_data);

  } else if constexpr (I2cMethod::kSmbus == method) {  // I2C SMBUS

    auto typed_data = static_cast<i2c_rdwr_smbus_data*>(data);
    return SmbusRW<true>(typed_data);  // write for false
  }
}

// i2c core functions
int I2c::I2cOpen(const char* bus_name) {
  int fd;

  // Open i2c-bus devcice
  if ((fd = open(bus_name, O_RDWR)) == -1) {
    return -1;
  }
  return fd;
}

void I2c::I2cClose(int fd) {
  if (FdValid(fd)) {
    close(fd);
  }
}

void I2c::I2cResetThisBus() {
  I2cClose(fd_);
  fd_ = -1;
  speed_ = 0;
  func_ = -1;
  name_ = "";
  last_slave_addr_ = 0;
}

// i2c sub functions

int32_t I2c::I2cUpdateThisBusFunc() {
  int32_t funcs = -1;

  if (!ioctl(fd_, I2C_FUNCS, &funcs)) {  // failed
    return -1;
  }

  func_ = funcs;
  return func_;
}

bool I2c::I2cUpdateFuncAndCompare(int32_t func) {
  if (I2cUpdateThisBusFunc() < 0) return false;
  return ((func_ & func) == func);
}

// speed related

bool I2c::IsPiDistribution(std::string_view current_version) {
  return (current_version.find("raspi") != std::string::npos);
}

uint32_t I2c::UpdateI2cSpeedOnPi() {
  std::string file_path = "/sys/bus/i2c/devices/i2c-";
  std::string_view bus_name_s(name_);
  file_path += bus_name_s.back();
  file_path += "/of_node/clock-frequency";
  std::string cmd = "xxd -ps ";
  std::string result = lra::terminal_util::Execute(cmd.append(file_path).c_str());
  if (result.find("xxd") != std::string::npos) {  // cmd failed
    speed_ = 0;
  } else {  // change to decimal
    speed_ = std::stoul(result, nullptr, 16);
  }

  return speed_;
}

// destructor
I2c::~I2c() { I2cClose(fd_); }
}  // namespace lra::bus