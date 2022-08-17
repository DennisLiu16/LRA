// compile g++ i2c_smbus_test.cc -li2c -o i2c_smbus_test -O3

// 測試結果顯示 smbus 和 plain 因為使用的時鐘頻率一樣，所以沒有受到 100k 的限制
// 410x (us) / 16 bytes 平均每 I2C BYTE (30 us for 9 bits/byte -> 26.448/Byte) 的時間是 26.448 (us) [ioctl & smbus
// 差不多] (不含構造數據時間) 使用 400kbits/s -> 理論值是 20 (us)

#define GET_AVG

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>

#ifdef __cplusplus
extern "C" {
#include <cstdio>
#else
#include <stdio.h>
#endif
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#ifdef __cplusplus
}
#endif

int main() {
  int fd = open("/dev/i2c-1", O_RDWR);

  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  uint8_t val = 0;
#ifndef GET_AVG
  start = std::chrono::high_resolution_clock::now();
  ioctl(fd, I2C_SLAVE, 0x70);
  i2c_smbus_write_byte(fd, 0x1);

  // smbus single byte read
  int ret = i2c_smbus_read_byte(fd);
  end = std::chrono::high_resolution_clock::now();

  printf("ret: %d\n", ret);
  printf("%lu (ns)\n", (end - start).count());

  // plain i2c single read
  start = std::chrono::high_resolution_clock::now();

  i2c_msg ioctl_msg{.addr = 0x70, .flags = 0 | I2C_M_RD, .len = 1, .buf = &val};

  i2c_rdwr_ioctl_data ioctl_data{.msgs{&ioctl_msg}, .nmsgs{1}};
  ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_data);
  end = std::chrono::high_resolution_clock::now();

  printf("ret: %d\n", val);
  printf("%lu (ns)\n", (end - start).count());

#endif
#ifdef GET_AVG
  uint64_t total = 0;
  uint32_t cycle = 100;
  // plain one byte write
  for (uint32_t i = 0; i < cycle; i++) {
    start = std::chrono::high_resolution_clock::now();
    uint8_t num = 1;
    i2c_msg ioctl_msg_w{.addr = 0x70, .flags = 0, .len = 1, .buf = &num};

    i2c_rdwr_ioctl_data ioctl_data_w{.msgs{&ioctl_msg_w}, .nmsgs{1}};
    ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_data_w);
    end = std::chrono::high_resolution_clock::now();
    total += (end - start).count();
  }
  printf("single byte write avg: %lu (ns)\n", total / cycle);

  // plain i2c 16 bytes write
  total = 0;

  for (uint32_t i = 0; i < cycle; i++) {
    uint8_t tmp_w_buf[16]{0};
    tmp_w_buf[14] = 0x2;
    tmp_w_buf[15] = 0x1;

    start = std::chrono::high_resolution_clock::now();
    i2c_msg ioctl_msg_w{.addr = 0x70, .flags = 0, .len = sizeof(tmp_w_buf), .buf = tmp_w_buf};

    i2c_rdwr_ioctl_data ioctl_data_w{.msgs{&ioctl_msg_w}, .nmsgs{1}};
    ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_data_w);
    end = std::chrono::high_resolution_clock::now();
    total += (end - start).count();
  }

  printf("mutiple byte write avg: avg: %lu (ns)\n", total / cycle);
#endif
#ifndef GET_AVG
  const uint8_t reg_iaddr = 0x04;
  const uint8_t drv = 0x5a;

  // smbus mutiple write test
  start = std::chrono::high_resolution_clock::now();

  const uint8_t smbus_w_buf[8] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8};
  uint8_t smbus_r_buf[16] = {0};

  ioctl(fd, I2C_SLAVE, drv);
  i2c_smbus_write_i2c_block_data(fd, reg_iaddr, sizeof(smbus_w_buf), smbus_w_buf);
  start = std::chrono::high_resolution_clock::now();
  // smbus mutiple read test
  i2c_smbus_read_i2c_block_data(fd, reg_iaddr, sizeof(smbus_r_buf), smbus_r_buf);

  end = std::chrono::high_resolution_clock::now();

  printf("smbus read buf: ");
  for (int i = 0; i < sizeof(smbus_r_buf); i++) {
    printf("%d ", *(smbus_r_buf + i));
  }
  printf("\n");
  printf("%lu (us)\n", (end - start).count());
  printf("\n");

  // plain mutiple write test

  uint8_t plain_w_buf[9] = {reg_iaddr, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1};  // write from 0x04
  uint8_t plain_r_buf[16] = {0};

  i2c_msg ioctl_w_msg{.addr = drv, .flags = 0, .len = sizeof(plain_w_buf), .buf = plain_w_buf};

  i2c_rdwr_ioctl_data ioctl_w_data{.msgs{&ioctl_w_msg}, .nmsgs{1}};
  ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_w_data);

  i2c_msg ioctl_r_msg[2];
  ioctl_r_msg[0].addr = drv;
  ioctl_r_msg[0].buf = const_cast<uint8_t*>(&reg_iaddr);
  ioctl_r_msg[0].flags = 0;
  ioctl_r_msg[0].len = sizeof(reg_iaddr);

  ioctl_r_msg[1].addr = drv;
  ioctl_r_msg[1].buf = plain_r_buf;
  ioctl_r_msg[1].flags = 0 | I2C_M_RD;
  ioctl_r_msg[1].len = sizeof(plain_r_buf);
  i2c_rdwr_ioctl_data ioctl_r_data{.msgs{ioctl_r_msg}, .nmsgs{2}};

  start = std::chrono::high_resolution_clock::now();
  ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_r_data);
  end = std::chrono::high_resolution_clock::now();

  printf("plain read buf: ");
  for (int i = 0; i < sizeof(plain_r_buf); i++) {
    printf("%d ", *(plain_r_buf + i));
  }
  printf("\n");
  printf("%lu (us)\n", (end - start).count());
  printf("\n");

  const uint8_t smbus_recover_buf[8] = {0};
  ioctl(fd, I2C_SLAVE, drv);
  i2c_smbus_write_i2c_block_data(fd, reg_iaddr, 8, smbus_recover_buf);
#endif
}