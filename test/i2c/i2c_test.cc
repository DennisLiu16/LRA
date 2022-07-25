
#include "i2c.h"
#include <sys/ioctl.h>


#include <chrono>
#include <cstdio>

#define STRING(s) #s

#include <array>
#include <memory>
#include <string>
#include <iostream>

std::string Execute(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

int main() {
  int fd = i2c_open("/dev/i2c-1");
  int funcs = -1;
  char buf[50];
  I2CDevice dev{.bus = fd, .addr = 0x5a, .tenbit = 0, .page_bytes = 8, .iaddr_bytes = 1};
  printf("%d\n", fd);
  i2c_ioctl_read(&dev, 0x0, buf, 33);
  printf("%x\n", buf[0]);
  i2c_read(&dev, 0x0, buf, 33);
  printf("%x\n", buf[0]);

  // get func
  int r = ioctl(fd, I2C_FUNCS, &funcs);

  if (funcs & I2C_FUNC_I2C)
    ;
  printf("%s", STRING(I2C_FUNC_I2C));

  i2c_ioctl_read(&dev, 0x0, buf, 33);

  // ioctl cost
  auto start = std::chrono::high_resolution_clock::now();
  int ten_bit = 1;
  int ten_init = 0;
  for (int i = 0; i < 10000; i++) {
    if (ten_bit != ten_init) {
      ioctl(fd, I2C_TENBIT, ten_init);
      ten_init = ten_bit;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  printf("cost %.3f ms\n", (end - start).count() / 1e6);

  // read test
  std::string file_path = "/sys/bus/i2c/devices/i2c-";
  std::string bus_name_s("/dev/i2c-1");
  file_path += bus_name_s.back();
  file_path += "/of_node/clock-frequency";
  std::string cmd = "xxd -ps ";
  std::string result = Execute(cmd.append(file_path).c_str());
  uint32_t speed_ = std::stoul(result, nullptr, 16);
  std::cout << "speed: " << speed_ << std::endl;

  i2c_close(dev.bus);

  // to test non Register input
}