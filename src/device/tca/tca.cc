#include <device/tca/tca.h>

namespace lra::device {
Tca9548a::Tca9548a(const I2cDeviceInfo& info) { info_ = info; }

bool Tca9548a::Init(I2cAdapter_S init_s) {

  // TODO: Log set to device ...
  init_s.dev_info_ = &info_;

  return adapter_.Init(init_s);
}

ssize_t Tca9548a::Write(const uint8_t& addr, const uint8_t* val, const uint32_t& len) {
  // val will never be negative number
  // Write(const uint8_t&, const uint8_t*, uint32_t len)
  return adapter_.Write(addr, val, len);
}

ssize_t Tca9548a::Read(const uint8_t& addr, uint8_t* val, const uint16_t& len) { return adapter_.Read(addr, val, len);}

std::vector<uint8_t> Tca9548a::Read(const uint8_t& addr, const uint16_t& len) {
  std::vector<uint8_t> vec(len);
  Read(addr, vec.data(), len);
  return vec;
}

int16_t Tca9548a::Read(const uint8_t& addr) { 
  
  // you can use other integral type here to store read 1 byte value
  uint8_t val = 0;
  return (adapter_.Read(addr, val) == sizeof(uint8_t))
              ? val
              : std::to_underlying(Errors::kRead); 
}
}  // namespace lra::device