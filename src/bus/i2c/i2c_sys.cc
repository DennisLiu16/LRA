#include <i2c_sys.h>
#include <util/log/log.h>

#if (SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_OFF)
#include <cstdio>
#endif

namespace lra::bus::i2c::sys {

static std::shared_ptr<lra::log_util::LogUnit> i2c_log = nullptr;  // related to I2cOpen()

int I2cOpen(const char* bus_name) {
  int fd;

  // Open i2c-bus devcice
  if ((fd = open(bus_name, O_RDWR)) == -1) {
    return -1;
  }

// TODO: Suggest this logunit only be created when system will use I2C bus.
// make sure AddDefaultLogger
#if SPDLOG_ACTIVE_LEVEL != SPDLOG_LEVEL_OFF
  i2c_log = lra::log_util::LogUnit::CreateLogUnit("i2c_sys");
#endif

  return fd;
}

void I2cClose(int fd) {
  if (I2cFdValid(fd)) {
    close(fd);
  }
}

int I2cSelect(int fd, uint16_t dev_addr, bool is_tenbit) {
  static int __fd = -1;
  static bool __is_tenbit = is_tenbit;
  static uint16_t __dev_addr = dev_addr;

  if (__fd != fd) {  // different bus, update all and leave
    if (ioctl(fd, I2C_TENBIT, is_tenbit)) return -1;

    if (ioctl(fd, I2C_SLAVE, dev_addr)) return -1;

    __fd = fd;
    __is_tenbit = is_tenbit;
    __dev_addr = dev_addr;

    return 0;
  }

  if (__is_tenbit != is_tenbit) {
    if (ioctl(fd, I2C_TENBIT, is_tenbit)) return -1;

    __is_tenbit = is_tenbit;
  }

  if (__dev_addr != dev_addr) {
    if (ioctl(fd, I2C_TENBIT, is_tenbit)) return -1;

    __dev_addr = dev_addr;
  }

  return 0;
}

int I2cResetDevice(I2cDevice* device) {
  device->delay_ = 100;
  device->bus_ = nullptr;
  device->info_ = { // with order
    .tenbit_ = false,
    .iaddr_bytes_ = 1,
    .page_bytes_ = 16,
    .addr_ = 0x0,
    .flags_ = 0,
  };
  device->dev_name_ = "";
}

// TODO: args mod to I2cBus
// get I2C funcs with controllable log
int I2cFuncCheckWithLog(const I2cDevice* device, bool log = false) {
  int funcs = -1;

  if (!ioctl(device->bus_->fd_, I2C_FUNCS, &funcs)) {  // failed
    return -1;
  }

  if (log) {
#if SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_OFF  // log system off, using printf
    const char* support = "    Support I2C func: %s\n";
    const char* not_support = "Not support I2C func: %s\n";
#define __I2C_SYS_LOG(__DESC, __FUNC) printf(__DESC, __FUNC);
    printf("I2C device: %s on bus: %s enter func check\n", device->dev_name_, device->bus_->bus_name_);
#else  // log system on
    constexpr const char* support = "    Support I2C func: {}\n";
    constexpr const char* not_support = "Not support I2C func: {}\n";
    i2c_log->Log(spdlog::level::info, "I2C device: {} on bus: {} enter func check\n", device->dev_name_,
                 device->bus_->bus_name_);
#define __I2C_SYS_LOG(__DESC, __FUNC) (i2c_log->Log(spdlog::level::info, __DESC, __FUNC))
#endif

#define FUNC_CHECK(__FUNC) (__I2C_SYS_LOG((funcs & __FUNC) ? support : not_support, __STRING(__FUNC)))

    FUNC_CHECK(I2C_FUNC_I2C);
    FUNC_CHECK(I2C_FUNC_10BIT_ADDR);
    FUNC_CHECK(I2C_FUNC_PROTOCOL_MANGLING);
    FUNC_CHECK(I2C_FUNC_SMBUS_PEC);
    FUNC_CHECK(I2C_FUNC_NOSTART);
    FUNC_CHECK(I2C_FUNC_SLAVE);
    FUNC_CHECK(I2C_FUNC_SMBUS_BLOCK_PROC_CALL);
    FUNC_CHECK(I2C_FUNC_SMBUS_QUICK);
    FUNC_CHECK(I2C_FUNC_SMBUS_READ_BYTE);
    FUNC_CHECK(I2C_FUNC_SMBUS_WRITE_BYTE);
    FUNC_CHECK(I2C_FUNC_SMBUS_READ_BYTE_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_WRITE_BYTE_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_READ_WORD_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_WRITE_WORD_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_PROC_CALL);
    FUNC_CHECK(I2C_FUNC_SMBUS_READ_BLOCK_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_WRITE_BLOCK_DATA);
    FUNC_CHECK(I2C_FUNC_SMBUS_READ_I2C_BLOCK);
    FUNC_CHECK(I2C_FUNC_SMBUS_WRITE_I2C_BLOCK);
    FUNC_CHECK(I2C_FUNC_SMBUS_HOST_NOTIFY);

#undef FUNC_CHECK(__FUNC)
#undef __I2C_SYS_LOG(__DESC, __FUNC)
  }
  return funcs;
}

bool I2cFuncCompare(const I2cDevice* device, int func) {
  int funcs = I2cFuncCheckWithLog(device);
  return ((funcs & func) == func);
}

// write related functions
size_t I2cIoctlRead(const I2cDevice* device, void* buf, size_t len, uint32_t iaddr = 0x0) {
  if (len == 0) {  // TODO: warning len wrong
    return len;
  }

  static struct i2c_msg ioctl_msg[2];
  static struct i2c_rdwr_ioctl_data ioctl_data;
  uint16_t flags = device->info_.tenbit_ ? (device->info_.flags_ | I2C_M_TEN) : device->info_.flags_;

  memset(ioctl_msg, 0, sizeof(ioctl_msg));
  memset(&ioctl_data, 0, sizeof(ioctl_data));

  // Target has internal address
  if (device->info_.iaddr_bytes_) {
    __u8 _iaddr[device->info_.iaddr_bytes_];
    memset(_iaddr, 0, sizeof(_iaddr));
    I2cInternalAddrConvert(iaddr, device->info_.iaddr_bytes_, _iaddr);

    // First message is write internal address
    ioctl_msg[0].len = device->info_.iaddr_bytes_;
    ioctl_msg[0].addr = device->info_.addr_;
    ioctl_msg[0].buf = _iaddr;
    ioctl_msg[0].flags = flags;

    // Package to i2c message to operation i2c device
    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = ioctl_msg;
  }

  // Target doesn't have internal address (only one register or not support specific address R/W)
  else {
    // Package to i2c message to operation i2c device
    ioctl_data.nmsgs = 1;
    ioctl_data.msgs = (ioctl_msg + 1);
  }

  // Data frame
  ioctl_msg[1].len = len;
  ioctl_msg[1].addr = device->info_.addr_;
  ioctl_msg[1].buf = (__u8*)buf;
  ioctl_msg[1].flags = flags | I2C_M_RD;

  /* Using ioctl interface operation i2c device */
  if (ioctl(device->bus_->fd_, I2C_RDWR, (unsigned long)&ioctl_data) == -1) {
    if (i2c_log) {
      // TODO: LogGlobal
      i2c_log->Log(spdlog::level::err, "I2C device: {} on {} ioctl write to internal address: {:#0x} failed\n",
                   device->dev_name_, device->bus_->bus_name_, iaddr);
    } else {
      perror("Ioctl read i2c error:");
    }
    return -1;
  }

  return len;
}

ssize_t I2cIoctlWrite(const I2cDevice* device, const void* buf, size_t len, uint32_t iaddr = 0x0) {
  if (len == 0) {  // TODO: warning len wrong
    return len;
  }

  ssize_t remain = len;
  size_t size = 0, cnt = 0;
  const __u8* buffer = (__u8*)buf;
  uint32_t delay = device->delay_;
  uint16_t flags = device->info_.tenbit_ ? (device->info_.flags_ | I2C_M_TEN) : device->info_.flags_;

  static struct i2c_msg ioctl_msg;
  static struct i2c_rdwr_ioctl_data ioctl_data;
  __u8 tmp_buf[device->info_.page_bytes_ + device->info_.iaddr_bytes_];

  while (remain > 0) {
    // Align page_bytes
    size = getI2cWriteSize(iaddr, remain, device->info_.page_bytes_);

    // Convert i2c internal address
    memset(tmp_buf, 0, sizeof(tmp_buf));
    I2cInternalAddrConvert(iaddr, device->info_.iaddr_bytes_, tmp_buf);

    // Connect write data after device internal address
    memcpy(tmp_buf + device->info_.iaddr_bytes_, buffer, size);

    // Fill kernel ioctl i2c_msg
    memset(&ioctl_msg, 0, sizeof(ioctl_msg));
    memset(&ioctl_data, 0, sizeof(ioctl_data));

    ioctl_msg.len = device->info_.iaddr_bytes_ + size;
    ioctl_msg.addr = device->info_.addr_;
    ioctl_msg.buf = tmp_buf;
    ioctl_msg.flags = flags;  // actually (flags | 0)
    ioctl_data.nmsgs = 1;
    ioctl_data.msgs = &ioctl_msg;

    if (ioctl(device->bus_->fd_, I2C_RDWR, (unsigned long)&ioctl_data) == -1) {
      // TODO: Log
      perror("Ioctl write i2c error:");
      return -1;
    }

    cnt += size;
    iaddr += size;
    buffer += size;
    remain -= size;
    if (remain > 0)
      // XXX: need this?
      // TODO: followed read?
      I2cDelay(delay);
  }
  return cnt;
}

void I2cDelay(uint32_t usec) {  // loose wait
  usleep(usec);
}

void I2cInternalAddrConvert(uint32_t iaddr, uint32_t len, __u8* addr) {
  union {
    unsigned int iaddr;
    unsigned char caddr[4];
  } convert;

  /* I2C internal address order is big-endian, same with network order */
  convert.iaddr = htonl(iaddr);

  /* Copy address to addr buffer */
  __u8 i = len - 1;
  __u8 j = 3;

  while (i >= 0 && j >= 0) {
    addr[i--] = convert.caddr[j--];
  }
}

}  // namespace lra::bus::i2c::sys