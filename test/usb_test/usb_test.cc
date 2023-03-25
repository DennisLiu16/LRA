#include <util/log/logunit.h>

#include <filesystem>
#include <iostream>

/* run this installation before use */
// sudo apt-get install libusb-1.0-0-dev
#include <libudev.h>
#include <libusb-1.0/libusb.h>

/* depend on libserial, 從官網下載到新的 repo 完成安裝，記得將 Test 關閉 (Google Test issue) */
#include <libserial/SerialPort.h>

namespace fs = std::filesystem;
using namespace LibSerial;

// msg string -- OUT
#define LRA_USB_OUT_INIT_STR ("LAB602 LRA USB OUT init\r\n")
#define LRA_USB_OUT_RESET_STM32_STR ("LAB602 LRA USB OUT STM32 Reset\r\n")

// msg string -- IN
#define LRA_USB_IN_INIT_STR ("LAB602 LRA USB IN init\r\n")
#define LRA_USB_IN_RESET_STM32_STR ("LAB602 LRA USB IN STM32 Reset\r\n")

typedef struct {
  uint16_t VID;
  uint16_t PID;
  std::string path;
  std::string product_name;
} USB_INFO_t;

typedef enum {
  LRA_USB_OUT_INIT_DL = sizeof(LRA_USB_OUT_INIT_STR) - 1,
  LRA_USB_OUT_UPDATE_PWM_DL = 0x01,
  LRA_USB_OUT_UPDATE_REG_DL = 0x01,
  LRA_USB_OUT_RESET_REG_DL = 0x01,
  LRA_USB_OUT_RESET_STM32_DL = sizeof(LRA_USB_OUT_RESET_STM32_STR) - 1,
} LRA_USB_Out_DL_t;

typedef enum {
  LRA_USB_IN_INIT_DL = sizeof(LRA_USB_IN_INIT_STR) - 1,
  LRA_USB_IN_UPDATE_PWM_DL = 0x01,
  LRA_USB_IN_UPDATE_REG_DL = 0x01,
  LRA_USB_IN_RESET_REG_DL = 0x01,
  LRA_USB_IN_RESET_STM32_DL = sizeof(LRA_USB_IN_RESET_STM32_STR) - 1,  // no return
} LRA_USB_In_DL_t;

typedef enum {
  LRA_USB_CMD_INIT = 0x00,
  LRA_USB_CMD_UPDATE_PWM = 0x01,
  LRA_USB_CMD_UPDATE_REG = 0x02,  // TODO: 更新哪一個的 register 放在 data 裡面好了
  LRA_USB_CMD_RESET_REG = 0x03,
  LRA_USB_CMD_RESET_STM32 = 0x04,
} LRA_USB_Cmd_t;

static std::vector<USB_INFO_t> get_all_usb_info();

int main() {
  spdlog::fmt_lib::print("Start to get all ttyUSB* and ttyACM* path\n");

  SerialPort stm32_usb;
  // std::vector<std::string> availablePorts = stm32_usb.GetAvailableSerialPorts();

  /* get all ttyUSB and ttyACM path */
  std::vector<std::string> usb_tty_path;

  for (auto &p : fs::directory_iterator("/dev/")) {
    if (p.is_character_file()) {
      auto device_path = p.path().string();
      if (device_path.find("ttyUSB") != std::string::npos || device_path.find("ttyACM") != std::string::npos) {
        usb_tty_path.push_back(device_path);
      }
    }
  }

  size_t index = 0;
  for (auto &path : usb_tty_path) {
    spdlog::fmt_lib::print("{}: path is {}\n", index, path);
    index++;
  }

  uint32_t usr_input;
  std::cin >> usr_input;

  if (usr_input < usb_tty_path.size()) {
    try {
      stm32_usb.Open(usb_tty_path[usr_input].c_str());
      stm32_usb.SetDTR(true);

      /* send init key string */
      uint8_t cmd = LRA_USB_CMD_INIT;
      uint16_t len = LRA_USB_OUT_INIT_DL;
      std::string data = LRA_USB_OUT_INIT_STR;

      /* set vector */
      uint8_t len_h = len >> 8;
      uint8_t len_l = len;
      std::vector<uint8_t> out_header{cmd, len_h, len_l};
      std::vector<uint8_t> out_data{data.begin(), data.end()};
      LibSerial::DataBuffer out_msg;
      out_msg.reserve(out_header.size() + out_data.size());
      out_msg.insert(out_msg.end(), out_header.begin(), out_header.end());
      out_msg.insert(out_msg.end(), out_data.begin(), out_data.end());

      stm32_usb.Write(out_msg);

      /* wait for tx finished */
      stm32_usb.DrainWriteBuffer();

      spdlog::fmt_lib::print("data sent\n");

      uint32_t usb_timeout_ms = 100;
      std::string rs;
      try {
        // Create a function Read \r\n
        stm32_usb.ReadLine(rs, '\n', usb_timeout_ms);
        spdlog::fmt_lib::print("rs is {}\n", rs);

      } catch (const ReadTimeout &) {
        std::cerr << "The ReadByte() call has timed out." << std::endl;
      }

      sleep(5);  // init time
      /* read all */
      while (stm32_usb.IsDataAvailable()) {
        stm32_usb.ReadLine(rs, '\n', 1000);
        spdlog::fmt_lib::print("{}\n", rs);
      };

      cmd = LRA_USB_CMD_RESET_STM32;
      len = LRA_USB_OUT_RESET_STM32_DL;
      data = LRA_USB_OUT_RESET_STM32_STR;

      sleep(5);

      /* set vector */
      len_h = len >> 8;
      len_l = len;
      out_header = {cmd, len_h, len_l};
      out_data.assign(data.begin(), data.end());

      LibSerial::DataBuffer out_reset_msg;
      out_reset_msg.reserve(out_header.size() + out_data.size());
      out_reset_msg.insert(out_reset_msg.end(), out_header.begin(), out_header.end());
      out_reset_msg.insert(out_reset_msg.end(), out_data.begin(), out_data.end());

      /* send reset stm32 cmd */
      stm32_usb.Write(out_reset_msg);

      /* wait for tx finished */
      stm32_usb.DrainWriteBuffer();
      try {
        // Create a function Read \r\n
        stm32_usb.ReadLine(rs, '\n', 0);
        spdlog::fmt_lib::print("rs is {}\n", rs);

      } catch (const ReadTimeout &) {
        std::cerr << "The ReadByte() call has timed out." << std::endl;
      }

    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  /* test get all usb info */

  // std::vector<USB_INFO_t> infos = get_all_usb_info();

  // for (auto &info : infos) {
  //   spdlog::fmt_lib::print("PID: {}\n", info.PID);
  //   spdlog::fmt_lib::print("VID: {}\n", info.VID);
  //   spdlog::fmt_lib::print("Product name: {}\n", info.product_name);
  //   spdlog::fmt_lib::print("Open path: {}\n\n", info.path);
  // }

  const uint16_t pid = 48602;  // DRV_STM, product name
  const uint16_t vid = 1155;   // STM

  // USB_INFO_t usb_info = get_target_vid_pid_usb_info(pid, vid);
}

std::vector<USB_INFO_t> get_all_usb_info() {
  libusb_device **devs;
  libusb_context *ctx;
  ssize_t cnt;
  std::vector<USB_INFO_t> usb_infos;

  if (libusb_init(&ctx) < 0) {
    std::cerr << "Failed to initialize libusb\n";
    return usb_infos;
  }

  cnt = libusb_get_device_list(ctx, &devs);
  if (cnt < 0) {
    std::cerr << "Failed to get device list\n";
    libusb_exit(ctx);
    return usb_infos;
  }

  libusb_device *dev;
  ssize_t i = 0;

  while ((dev = devs[i++]) != nullptr) {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);

    USB_INFO_t usb_info;
    usb_info.VID = desc.idVendor;
    usb_info.PID = desc.idProduct;

    struct udev *udev = udev_new();
    if (!udev) {
      std::cerr << "Failed to initialize udev\n";
      libusb_free_device_list(devs, 1);
      libusb_exit(ctx);
      return usb_infos;
    }

    struct udev_device *udevice;
    udevice = udev_device_new_from_subsystem_sysname(udev, "usb", "usb_device");
    const char *path = udev_device_get_devnode(udevice);
    usb_info.path = path ? std::string(path) : "";

    udev_device_unref(udevice);
    udev_unref(udev);

    libusb_device_handle *handle;
    int ret = libusb_open(dev, &handle);
    if (ret == 0) {
      unsigned char product_name[256];
      libusb_get_string_descriptor_ascii(handle, desc.iProduct, product_name, sizeof(product_name));
      usb_info.product_name = std::string(reinterpret_cast<char *>(product_name));
      libusb_close(handle);
    }

    else {
      spdlog::fmt_lib::print("libusb_open: {} failed, err number is: {}\n", i, ret);
    }

    usb_infos.push_back(usb_info);
  }

  libusb_free_device_list(devs, 1);
  libusb_exit(ctx);
  return usb_infos;
}
