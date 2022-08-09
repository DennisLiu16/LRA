#include <bus_adapter/i2c_adapter/i2c_adapter.h>

#include <vector>

namespace lra::bus_adapter::i2c {

bool I2cAdapter::InitImpl(const char* adapter_name) {
  I2cAdapterInit_S s;
  s.name_ = adapter_name;
  info_ = std::move(s);
  logunit_ = ::lra::log_util::LogUnit::CreateLogUnit(info_.name_);
  return true;
}

bool I2cAdapter::InitImpl(const I2cAdapterInit_S& init_s) {
  info_ = init_s;
  logunit_ = ::lra::log_util::LogUnit::CreateLogUnit(info_.name_);
  return true;
}

// case 2
// val won't be modified because const will be declaration in next layer (in bus/i2c.h) WriteMultiImpl
ssize_t I2cAdapter::WriteImpl(const uint64_t& iaddr, const uint8_t* val, const uint16_t len) {
  if (!I2cInternalAddrCheck(info_.dev_info_->iaddr_bytes_, iaddr)) {
    // TODO:Logerr(addr len > dev_info_->iaddr_bytes_);
    return 0;
  }

  ssize_t ret_size = 0;

  // deal with datas
  if (info_.method_ == I2c::I2cMethod::kPlain) {
    if (I2cPlainCheckFail()) {
      // TODO: Log: use plain to communicate with 10 bits address, waiting for implementation
      return 0;
    }
    i2c_msg ioctl_msg;
    i2c_rdwr_ioctl_data ioctl_data{.msgs{&ioctl_msg}, .nmsgs{1}};
    uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;
    uint8_t tmp_buf[len + info_.dev_info_->iaddr_bytes_] = {0};

    if (info_.dev_info_->iaddr_bytes_ <= 4) {
      I2cInternalAddrConvert(iaddr, info_.dev_info_->iaddr_bytes_, tmp_buf);
    } else {
      assert(info_.dev_info_->iaddr_bytes_ <= 4 && "iaddr_bytes > 4, no implementation found");
    }

    // XXX:copy val to tmp_buf
    memcpy(tmp_buf + info_.dev_info_->iaddr_bytes_, val, len);

    ioctl_msg.len = sizeof(tmp_buf);
    ioctl_msg.addr = info_.dev_info_->addr_;  // slave address
    ioctl_msg.buf = tmp_buf;
    ioctl_msg.flags = flags;

    ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kPlain>(&ioctl_data);
  } else if (info_.method_ == I2c::I2cMethod::kSmbus) {
    if (I2cSmbusCheckFail()) {  // smbus check failed
      // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
      return 0;
    }

    i2c_rdwr_smbus_data smbus_data{// XXX: const_cast
                                   .no_internal_reg_{(info_.dev_info_->iaddr_bytes_) ? false : true},
                                   .command_{(uint8_t)iaddr},
                                   .len_{len},
                                   .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                   .value_{const_cast<uint8_t*>(val)}};

    ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kSmbus>(&smbus_data);
  } else
    assert(false && "I2C method unset");

  // delay ...
  // XXX: ret_size condi?
  if (ret_size > 0) I2cDelay(info_.delay_);

  return ret_size;
}

// case 3. (addr, vector)
ssize_t I2cAdapter::WriteImpl(const uint64_t& iaddr, const std::vector<uint8_t>& val) {
  if (!I2cInternalAddrCheck(info_.dev_info_->iaddr_bytes_, iaddr)) {
    // TODO:Logerr(addr len > dev_info_->iaddr_bytes_);
    return 0;
  }

  ssize_t ret_size = 0;

  if (info_.method_ == I2c::I2cMethod::kPlain) {
    if (I2cPlainCheckFail()) {
      // TODO: Log: use plain to communicate with 10 bits address, waiting for implementation
      return 0;
    }
    i2c_msg ioctl_msg;
    i2c_rdwr_ioctl_data ioctl_data{.msgs{&ioctl_msg}, .nmsgs{1}};
    uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;
    uint8_t tmp_buf[val.size() + info_.dev_info_->iaddr_bytes_] = {0};  // XXX: use VLA

    if (info_.dev_info_->iaddr_bytes_ <= 4) {
      I2cInternalAddrConvert(iaddr, info_.dev_info_->iaddr_bytes_, tmp_buf);
    } else {
      assert(info_.dev_info_->iaddr_bytes_ <= 4 && "iaddr_bytes > 4, no implementation found");
    }

    // XXX:copy val to tmp_buf
    memcpy(tmp_buf + info_.dev_info_->iaddr_bytes_, val.data(), val.size());

    ioctl_msg.len = sizeof(tmp_buf);
    ioctl_msg.addr = info_.dev_info_->addr_;  // slave address
    ioctl_msg.buf = tmp_buf;
    ioctl_msg.flags = flags;

    ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kPlain>(&ioctl_data);
  } else if (info_.method_ == I2c::I2cMethod::kSmbus) {
    if (I2cSmbusCheckFail()) {  // smbus check failed
      // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
      return 0;
    }

    i2c_rdwr_smbus_data smbus_data{.no_internal_reg_{(info_.dev_info_->iaddr_bytes_) ? false : true},
                                   .command_{(uint8_t)iaddr},
                                   .len_{val.size()},
                                   .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                   // XXX: const_cast
                                   .value_{const_cast<uint8_t*>(val.data())}};

    ret_size = info_.bus_->WriteMulti<I2c::I2cMethod::kSmbus>(&smbus_data);
  } else
    assert(false && "I2C method unset");

  // delay if succeeded
  // XXX: ret_size condi?
  if (ret_size > 0) I2cDelay(info_.delay_);

  return ret_size;
}

// read functions

// case 3
ssize_t I2cAdapter::ReadImpl(const int64_t& iaddr, uint8_t* val, const uint16_t& len) {
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

    uint16_t flags = info_.dev_info_->tenbit_ ? (info_.dev_info_->flags_ | I2C_M_TEN) : info_.dev_info_->flags_;

    struct i2c_msg ioctl_msg[2]{0};
    struct i2c_rdwr_ioctl_data ioctl_data {
      .msgs { ioctl_msg }
    };

    if (info_.dev_info_->iaddr_bytes_ > 0) {  // has internal address

      uint8_t converted_iaddr[info_.dev_info_->iaddr_bytes_] = {0};

      if (info_.dev_info_->iaddr_bytes_ <= 4) {
        I2cInternalAddrConvert(iaddr, info_.dev_info_->iaddr_bytes_, converted_iaddr);
      } else {
        assert(info_.dev_info_->iaddr_bytes_ <= 4 && "iaddr_bytes > 4, no implementation found");
      }

      // write internal address
      ioctl_msg[0].len = info_.dev_info_->iaddr_bytes_;
      ioctl_msg[0].addr = info_.dev_info_->addr_;
      ioctl_msg[0].buf = converted_iaddr;
      ioctl_msg[0].flags = flags;

      // read data
      ioctl_msg[1].len = len;
      ioctl_msg[1].addr = info_.dev_info_->addr_;
      ioctl_msg[1].buf = val;
      ioctl_msg[1].flags = flags | I2C_M_RD;
      ioctl_data.nmsgs = 2;
    } else {
      ioctl_msg[0].len = len;
      ioctl_msg[0].addr = info_.dev_info_->addr_;
      ioctl_msg[0].buf = val;
      ioctl_msg[0].flags = flags | I2C_M_RD;
      ioctl_data.nmsgs = 1;
    }

    ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kPlain>(&ioctl_data);

  } else if (info_.method_ == I2c::I2cMethod::kSmbus) {
    if (I2cSmbusCheckFail()) {  // smbus check failed
      // TODO: Log: use smbus to communicate with 10 bits address, waiting for implementation
      return 0;
    }

    i2c_rdwr_smbus_data smbus_data{.no_internal_reg_{(info_.dev_info_->iaddr_bytes_) ? false : true},
                                   .command_{(uint8_t)iaddr},
                                   .len_{len},
                                   .slave_addr_{(uint8_t)info_.dev_info_->addr_},
                                   .value_{val}};

    ret_size = info_.bus_->ReadMulti<I2c::I2cMethod::kSmbus>(&smbus_data);

  } else {
    assert(false && "I2C method unset");
  }

  // FIXME: if len > 32, ret_size == 32, fix this later
  // TODO: Delay need size check?
  if (ret_size == len) {
    I2cDelay(info_.delay_);
  }

  return ret_size;
}

// convert to big-endain and store into to_buf
// e.g. covert 0x01020304 -> to_buf[0x01, 0x02, 0x03, 0x04, ...];
void I2cAdapter::I2cInternalAddrConvert(uint32_t iaddr, uint8_t nbyte, uint8_t* to_buf) {
  union {
    unsigned int iaddr;
    unsigned char caddr[4];
  } convert;

  // I2C internal address order is big-endian, same with network order
  convert.iaddr = htonl(iaddr);

  /* Copy address to addr buffer */
  int16_t i = nbyte - 1;
  int16_t j = 3;

  while (i >= 0 && j >= 0) {
    to_buf[i--] = convert.caddr[j--];
  }
}

bool I2cAdapter::I2cSmbusCheckFail() {
  // 10-bit slave address is not allowed
  // iaddr_bytes only accepts 1 byte
  // TODO: add len check (__u8)
  return (info_.dev_info_->tenbit_) || (info_.dev_info_->iaddr_bytes_ > sizeof(uint8_t));
}

bool I2cAdapter::I2cPlainCheckFail() {
  // bus has I2C function
  // user choose Plain R/W
  // TODO: add len check (__u16)
  return !(info_.bus_->func_ & I2C_FUNC_I2C);
}
}  // namespace lra::bus_adapter::i2c