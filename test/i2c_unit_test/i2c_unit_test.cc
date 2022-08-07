#define I2C_UNIT_TEST // for bus/i2c.h -> disable writing to real device

// FIXME: if include <fmt/core.h>, something goes wrong (maybe conflict with internal fmt lib)
#include <bus_adapter/i2c_adapter/i2c_adapter.h>
#include <device/device.h>
#include <memory/registers/registers.h>

#include <chrono>

using spdlog::fmt_lib::print;

using lra::memory::registers::Register_16;
using lra::memory::registers::Register_32;
using lra::memory::registers::Register_64;
using lra::memory::registers::Register_8;

using lra::bus::I2c;
using lra::bus_adapter::i2c::I2cAdapter_S;
using lra::device::I2cAdapter;
using lra::device::I2cDeviceInfo;

int main() {
  /* init - deviceinfo */
  I2cDeviceInfo dev1;
  dev1.addr_ = 0x5a;
  dev1.flags_ = 0;
  dev1.iaddr_bytes_ = 1;
  dev1.page_bytes_ = 32;
  dev1.tenbit_ = false;

  /* init - bus */
  I2c i2c_bus;
  i2c_bus.Init("/dev/i2c-1");

  // maybe add another bus

  /* init - busadapter */
  I2cAdapter_S i2c_adapter_init_s;
  i2c_adapter_init_s.bus_ = std::make_shared<I2c>(i2c_bus);
  i2c_adapter_init_s.delay_ = 100;
  i2c_adapter_init_s.dev_info_ = &dev1;
  i2c_adapter_init_s.method_ = I2c::I2cMethod::kPlain;
  I2cAdapter i2c_adapter;
  i2c_adapter.Init(i2c_adapter_init_s);

  /* init - register */
  constexpr static Register_8 R8{0x0, 0x0};
  constexpr static Register_16 R16{0x0, 0x0};
  constexpr static Register_32 R32{0x0, 0x0};
  constexpr static Register_64 R64{0x0, 0x0};

  // correct buf size to decltype(R8)::val_t in i2c_adapter.h (int in default, that is 4 bytes, but R8 is 1 byte data len)

  
  /* write function test - register*/
  #ifdef I2C_UNIT_TEST
  print("Write: Register 8\n");
  i2c_adapter.Write(R8,  0x01);
  print("Write: Register 16\n");
  i2c_adapter.Write(R16, 0x0102);
  print("Write: Register 32\n");
  i2c_adapter.Write(R32, 0x01020304);
  print("Write: Register 64\n");
  i2c_adapter.Write(R64, 0x0102030405060708);
  print("Write: Register 8 - should warning oversize\n");
  i2c_adapter.Write(R8, 0x0102); // should get a warning: cast to 0x02

  /* write function test - array*/
  // ssize_t WriteImpl(const uint64_t& iaddr, uint8_t* val, const uint16_t len);
  uint8_t buf[2] = {0x11,0x22};
  print("Write: array\n");
  i2c_adapter.Write(R8.addr_, buf, sizeof(buf));
  i2c_adapter.Write(0x0, buf, sizeof(buf));
  std::vector <uint8_t>vec = {0x33,0x44};
  i2c_adapter.Write(R8.addr_, vec.data(), vec.capacity());

  /* write function test - vector*/
  print("Write: vector\n");
  i2c_adapter.Write(R8.addr_, vec);
  #endif

  /* record */
  // TODO: Write to Hackmd
  // GENERRATE DATA
  // PlainRW 
  //  Register_8: 18 (us)
  // 
}