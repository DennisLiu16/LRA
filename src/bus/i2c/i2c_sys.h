#ifndef LRA_BUS_I2C_SYS_H_
#define LRA_BUS_I2C_SYS_H_
// The MIT License (MIT)

// Copyright (c) 2014 Amaork (original version)

// modify from: https://github.com/amaork/libi2c
// Copyright (c) 2022 Dennis

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// i2cdetect version:
// make sure your i2c-tools version: i2cdetect -V
// ubuntu 20.04 -> version 4.1
// https://lishiwen4.github.io/bus/i2c-dev-interface

// smbus:
// smbus source code "smbus.c" : https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/
// usgae i2c_smbus_... https://www.cnblogs.com/lknlfy/p/3265122.html
// https://stackoverflow.com/questions/61657749/cant-compile-i2c-smbus-write-byte-on-raspberry-pi-4

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#ifdef __cplusplus
}
#endif

#include <bus/i2c/i2c.h>

namespace lra::bus::i2c::sys {

// This Version aborts sys write and read method

// funcs

/**
 * @brief i2c open function
 *
 * @param bus_name
 * @return int
 * @details You can use i2c-tools to check i2c buses on your machine
 *          (i2cdetect -l)
 */
int I2cOpen(const char* bus_name);

void I2cClose(int fd);

int I2cSelect(int fd, unsigned long dev_addr, bool is_tenbit);

int I2cResetDevice(I2cDevice* device);

static inline bool I2cFdValid(int fd) { return fcntl(fd, F_GETFL) != -1 || errno != EBADF; };

static inline size_t getI2cWriteSize(uint32_t iaddr, ssize_t remain, uint16_t page_bytes) {
  uint16_t base_iaddr = iaddr % page_bytes;
  return (base_iaddr + remain) > page_bytes ? page_bytes - base_iaddr : remain;
};

/**
 * @brief
 *
 * @param device
 * @param log default to false
 * @return int
 * @url: https://elixir.bootlin.com/linux/v5.4/source/include/uapi/linux/i2c.h#L90
 * @url: https://www.kernel.org/doc/html/latest/i2c/functionality.html
 */
int I2cFuncCheckWithLog(const I2cDevice* device, bool log = false);

bool I2cFuncCompare(const I2cDevice* device, int func);

size_t I2cIoctlRead(const I2cDevice* device, void* buf, size_t len, uint32_t iaddr);

ssize_t I2cIoctlWrite(const I2cDevice* device, const void* buf, size_t len, uint32_t iaddr);

static void I2cDelay(uint32_t usec);

void I2cInternalAddrConvert(uint32_t iaddr, uint32_t len, __u8* addr);
}  // namespace lra::bus::i2c::sys

#endif
