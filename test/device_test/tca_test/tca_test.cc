// #define I2C_UNIT_TEST  // this macro will disable plain write function (write only)

#include <bus_adapter/i2c_adapter/i2c_adapter.h>
#include <device/device_info.h>
#include <device/tca/tca.h>

#include <chrono>

using ::lra::bus::I2c;
using ::lra::bus_adapter::i2c::I2cAdapter_S;
using ::lra::device::I2cDeviceInfo;
using ::lra::device::Tca9548a;

int main() {
  // Deviceinfo
  I2cDeviceInfo info;
  info.addr_ = 0x70;
  info.flags_ = 0;
  info.iaddr_bytes_ = 0;  // should be 0
  info.page_bytes_ = 16;  // ?
  info.tenbit_ = false;

  // bus
  I2c i2c;
  i2c.Init("/dev/i2c-1");
  printf("I2C speed is: %d\n", i2c.speed_);

  // I2cAdapter init struct
  I2cAdapter_S init_s;
  init_s.bus_ = std::make_shared<I2c>(i2c);
  init_s.delay_ = 0;
  init_s.method_ = I2c::I2cMethod::kSmbus;
  init_s.name_ = "tca";

  /*test Plain 400k*/
  uint64_t total = 0;
  Tca9548a tca(info);  // info_ = info
  tca.Init(init_s);    // init internal member I2cAdapter with public member info_

  // tca.Write(tca.CONTROL.addr_, {0x2, 0x4});  // write std::initializer ok
  uint32_t cycle = 10000;
  for (int i = 0; i < cycle; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    tca.Write(tca.CONTROL, 0x2);               // write std::initializer ok, test delay also (0 delay is ok)
    auto end = std::chrono::high_resolution_clock::now();
    total += (end - start).count();
  }

  printf("write one byte avg: %ld (ns)\n", total / cycle);

  total = 0;

  for (int i = 0; i < cycle; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    uint8_t tmp_w_buf[16]{0};
    tmp_w_buf[14] = 0x2;
    tmp_w_buf[15] = 0x1;

    tca.Write(
        tca.CONTROL(), tmp_w_buf,
        sizeof(tmp_w_buf));  // BUG: wtf, 其他write就可以 (when define I2C_UNIT_TEST)? -> 可能是在 src file 的關係?

    auto end = std::chrono::high_resolution_clock::now();
    total += (end - start).count();
  }
  tca.Write(tca.CONTROL, 0x2);               // write std::initializer ok, test delay also (0 delay is ok)
  printf("read value: %d\n", tca.Read(tca.CONTROL));
  tca.Write(tca.CONTROL, 0x1);               // write std::initializer ok, test delay also (0 delay is ok)
  printf("avg: %ld (ns)\n", total / cycle);
}