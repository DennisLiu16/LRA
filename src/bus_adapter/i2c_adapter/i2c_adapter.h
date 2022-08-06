#ifndef LRA_BUS_ADAPTER_I2C_H_
#define LRA_BUS_ADAPTER_I2C_H_

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

// TODO Note: what is slave address and register internal addressx
// TODO Note: do some driver check

// This layer is for the purposes of
// 1. delay after R/W
// 2. build data struct transfer to bus
// 3. check user device settings are correct

#include <bus/i2c/i2c.h>
#include <bus_adapter/bus_adapter.h>
#include <linux/i2c.h>

#include <array>
#include <memory>
#include <string_view>

namespace lra::bus_adapter::i2c {
using ::lra::bus::I2c;
using ::lra::bus::i2c_rdwr_smbus_data;
using ::lra::device::I2cDeviceInfo;

// struct, restore properties of i2c device
typedef struct I2cAdapterInit_S {  // Restore an I2C device's properties and bus, 8 byte alignment, total 40 bytes
  uint32_t delay_{100};            // 4 bytes, I2C internal operation delay, unit microseconds, usleep()
  I2c::I2cMethod method_{I2c::I2cMethod::kPlain};  // 4 bytes, I2c method
  const char* name_{""};                           // 8 bytes
  const I2cDeviceInfo* dev_info_{nullptr};         // 8 bytes, point to device's class member
  std::shared_ptr<I2c> bus_{nullptr};              // 16 bytes,

  I2cAdapterInit_S& operator=(const I2cAdapterInit_S& other) = default;  // copy assignment
  I2cAdapterInit_S& operator=(I2cAdapterInit_S&& other) = default;       // move assignment
} I2cAdapter_S;

/**
 * @brief The purposes of this class is to complete the data from device and make those data communicate to bus class.
 * Also restores the information about device's features related to bus, e.g. delay, internal address byte...
 *
 *
 */
class I2cAdapter : public BusAdapter<I2cAdapter> {
 public:
  // members
  I2cAdapter_S info_;

  // TODO: Factory with shared_ptr

 private:
  friend BusAdapter;

  // CRTP Impl, using template
  bool InitImpl(const char* adapter_name);

  bool InitImpl(const I2cAdapterInit_S& init_s);

  // Args' types in I2C
  // TODO: Write this in readme.md
  // iaddr is type uint64_t for Register_T use uint64_t to restore register address (general usage).
  // Linux:
  // Plain I2C -> __u16 for data length (2 bytes), __u16 for slave_address (7 or 10 bits), __u32 for iaddr (restricted
  // by htonl not ioctl, you can complete this part)
  // SMBus I2C -> __u8 for data length (1 byte), __u8 for slave_address (7 bits only), __u8 for iaddr (member command_)
  // SystemR/W I2C -> wait for implementation (I guess there's no limitation)

  /**
   * write function related interface
   * 1. (register, single val) e.g. val = (0x12's val << 8) | 0x13's val; starts from MSB
   * 2. (iaddr, val*, len)
   * 3. (iaddr, vector)
   * all return ssize_t (write how many bytes, -1 for failure)
   * plainR/W return iaddr + buf size, smbus return buf_size
   */

  // case 1. (register, single val)
  // XXX: assume all data can be writen at once
  template <is_register T, std::integral U>
  ssize_t WriteImpl(const T& reg, const U& val) {
    // TODO: Write a function to decrease repeat code for write WriteImpl
    // check iaddr len <= iaddr_bytes_ (make sure that iaddr is perfectly convertible)
    if (!I2cInternalAddrCheck(info_.dev_info_->iaddr_bytes_, reg.addr_)) {
      // TODO:Logerr(iaddr len > dev_info_->iaddr_bytes_);
      return 0;
    }

    ssize_t ret_size = 0;

    // call I2C plain method
    if (info_.method_ == I2c::I2cMethod::kPlain) {
      if (I2cPlainCheckFail()) {
        // TODO: Log: use plain to communicate with 10 bits address, waiting for implementation
        return 0;
      }

      i2c_msg ioctl_msg;
      i2c_rdwr_ioctl_data ioctl_data{.msgs{&ioctl_msg}, .nmsgs{1}};

      uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;
      // FIXME: U's type may be ullong for bitset converting problem
      uint8_t tmp_buf[sizeof(U) + info_.dev_info_->iaddr_bytes_] = {0};  // integral size + iaddr len

      // copy iaddr to tmp_buf
      if (info_.dev_info_->iaddr_bytes_ <= 4) {
        I2cInternalAddrConvert(reg.addr_, info_.dev_info_->iaddr_bytes_, tmp_buf);
      } else {
        assert("iaddr_bytes > 4, no implementation found");
      }

      RewriteIntegralToBigEndianArray(reg.bytelen_, val, tmp_buf + info_.dev_info_->iaddr_bytes_);  // tmp_buf with bias

      /*
        tmp_buf completes
        0x12 0x13 0x14: iaddr
        1    2    3   : val = 0x010203
        -> generate tmp_buf[0x12, 0x01, 0x02, 0x03];
      */

      // generate data
      ioctl_msg.len = sizeof(tmp_buf);
      ioctl_msg.addr = info_.dev_info_->addr_;  // slave address
      ioctl_msg.buf = tmp_buf;
      ioctl_msg.flags = flags;

      ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kPlain>(&ioctl_data);
    } else if (info_.method_ == I2c::I2cMethod::kSmbus) {  // call SMBus method
      if (I2cSmbusCheckFail()) {                           // smbus check failed
        // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
        return 0;
      }

      uint8_t tmp_buf[sizeof(U)] = {0};

      i2c_rdwr_smbus_data smbus_data{.command_{(uint8_t)reg.addr_},
                                     .len_{sizeof(U)},
                                     .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                     .value_{tmp_buf}};

      RewriteIntegralToBigEndianArray(reg.bytelen_, val, tmp_buf);

      ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kSmbus>(&smbus_data);

    } else {
      assert("I2C method unset");
    }

    // delay if succeeded
    // FIXME: > 0 ?
    if (ret_size > 0) I2cDelay(info_.delay_);

    return ret_size;
  }

  // case 2. (iaddr, val*, len)
  // val won't be modified because const will be declaration in next layer (in bus/i2c.h)
  ssize_t WriteImpl(const uint64_t& iaddr, uint8_t* val, const uint16_t len);

  // case 3. (iaddr, vector)
  // val won't be modified because const will be declaration in next layer (in bus/i2c.h)
  ssize_t WriteImpl(const uint64_t& iaddr, std::vector<uint8_t>& val);

  /**
   * read function related interface
   * 1. reg -> auto intx_t
   * 2. iaddr -> int16_t (negative for failure) [read one byte]
   * 3. (iaddr, val*, len) -> ssize_t
   */

  // case 1.
  template <is_register T, std::integral U>
  ssize_t ReadImpl(const T& reg, U& val) {
    if (!I2cInternalAddrCheck(info_.dev_info_->iaddr_bytes_, reg.addr_)) {
      // TODO:Logerr(iaddr len > dev_info_->iaddr_bytes_);
      return 0;
    }

    ssize_t ret_size = 0;
    uint8_t tmp_buf[sizeof(U)] = {0};

    if (info_.method_ == I2c::I2cMethod::kPlain) {
      if (I2cPlainCheckFail()) {
        // TODO: Log: use plain to communicate with 10 bits address, waiting for implementation
        return 0;
      }
      struct i2c_msg ioctl_msg[2];
      struct i2c_rdwr_ioctl_data ioctl_data {
        .msgs{ioctl_msg}, .nmsgs { 2 }
      };

      uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;
      uint8_t converted_iaddr[info_.dev_info_->iaddr_bytes_] = {0};

      if (info_.dev_info_->iaddr_bytes_ <= 4) {
        I2cInternalAddrConvert(reg.addr_, info_.dev_info_->iaddr_bytes_, converted_iaddr);
      } else {
        assert("iaddr_bytes > 4, no implementation found");
      }

      // write internal address
      ioctl_msg[0].len = info_.dev_info_->iaddr_bytes_;
      ioctl_msg[0].addr = info_.dev_info_->addr_;
      ioctl_msg[0].buf = converted_iaddr;
      ioctl_msg[0].flags = flags;

      // read data
      ioctl_msg[1].len = sizeof(tmp_buf);
      ioctl_msg[1].addr = info_.dev_info_->addr_;
      ioctl_msg[1].buf = tmp_buf;
      ioctl_msg[1].flags = flags | I2C_M_RD;

      ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kPlain>(&ioctl_data);

    } else if (info_.method_ == I2c::I2cMethod::kSmbus) {
      if (I2cSmbusCheckFail()) {  // smbus check failed
        // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
        return 0;
      }

      i2c_rdwr_smbus_data smbus_data{.command_{(uint8_t)reg.addr_},
                                     .len_{sizeof(U)},
                                     .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                     .value_{tmp_buf}};

      ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kSmbus>(&smbus_data);

    } else {
      assert("I2C method unset");
    }

    if (ret_size == sizeof(tmp_buf)) {  // ret_size of plain and smbus should be same in read mode
      // tmp_buf's size align U's
      Array2Integral_BE(val, tmp_buf);
      I2cDelay(info_.delay_);
    }

    return ret_size;
  }

  // case 2.
  // case 2 read one byte
  // extend user variable type (not only uint8_t&)
  ssize_t ReadImpl(const int64_t& iaddr, std::integral auto& val_r) {
    uint8_t val = 0;
    if (!I2cInternalAddrCheck(info_.dev_info_->iaddr_bytes_, iaddr)) {
      // TODO:Logerr(iaddr len > dev_info_->iaddr_bytes_);
      return 0;
    }

    ssize_t ret_size = 0;

    if (info_.method_ == I2c::I2cMethod::kPlain) {
      if (I2cPlainCheckFail()) {
        // TODO: Log: use plain to communicate with 10 bits address, waiting for implementation
        return 0;
      }
      struct i2c_msg ioctl_msg[2];
      struct i2c_rdwr_ioctl_data ioctl_data {
        .msgs{ioctl_msg}, .nmsgs { 2 }
      };

      uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;
      uint8_t converted_iaddr[info_.dev_info_->iaddr_bytes_] = {0};

      if (info_.dev_info_->iaddr_bytes_ <= 4) {
        I2cInternalAddrConvert(iaddr, info_.dev_info_->iaddr_bytes_, converted_iaddr);
      } else {
        assert("iaddr_bytes > 4, no implementation found");
      }

      // write internal address
      ioctl_msg[0].len = info_.dev_info_->iaddr_bytes_;
      ioctl_msg[0].addr = info_.dev_info_->addr_;
      ioctl_msg[0].buf = converted_iaddr;
      ioctl_msg[0].flags = flags;

      // read data
      ioctl_msg[1].len = sizeof(uint8_t);
      ioctl_msg[1].addr = info_.dev_info_->addr_;
      ioctl_msg[1].buf = &val;
      ioctl_msg[1].flags = flags | I2C_M_RD;

      ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kPlain>(&ioctl_data);

    } else if (info_.method_ == I2c::I2cMethod::kSmbus) {
      if (I2cSmbusCheckFail()) {  // smbus check failed
        // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
        return 0;
      }

      i2c_rdwr_smbus_data smbus_data{.command_{(uint8_t)iaddr},
                                     .len_{sizeof(uint8_t)},
                                     .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                     .value_{&val}};

      ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kSmbus>(&smbus_data);

    } else {
      assert("I2C method unset");
    }

    if (ret_size == sizeof(uint8_t)) {
      val_r = val;
      I2cDelay(info_.delay_);
    }

    return ret_size;
  }

  // case 3. (including vector.data())
  ssize_t ReadImpl(const int64_t& iaddr, uint8_t* val, const uint16_t& len);

  static inline size_t getI2cWriteSize(const uint32_t& iaddr, const ssize_t& remain, const uint16_t& page_bytes) {
    uint16_t base_iaddr = iaddr % page_bytes;
    return (base_iaddr + remain) > page_bytes ? page_bytes - base_iaddr : remain;
  };

  static inline void I2cDelay(uint32_t us) { usleep(us); }

  // check valid part of iaddr can be converted into (iaddr_bytes) type unsigned integral
  inline bool I2cInternalAddrCheck(const uint8_t& iaddr_bytes, const uint64_t& iaddr) {
    return (MakeMaskWithLen(iaddr_bytes) & iaddr) == iaddr;
  }

  void I2cInternalAddrConvert(uint32_t iaddr, uint8_t nbyte, uint8_t* to_buf);

  bool I2cSmbusCheckFail();

  bool I2cPlainCheckFail();

  // TODO: move to bus_adapter.h
  void RewriteIntegralToBigEndianArray(uint8_t nbytes, std::integral auto& val, void* buf);
};

}  // namespace lra::bus_adapter::i2c

#endif