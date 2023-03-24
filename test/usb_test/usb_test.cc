#include <util/log/logunit.h>

#include <filesystem>
#include <iostream>

/* run this installation before use */
// sudo apt-get install libusb-1.0-0-dev
#include <libusb-1.0/libusb.h>

/* depend on libserial, 從官網下載到新的 repo 完成安裝，記得將 Test 關閉 (Google Test issue) */
#include <libserial/SerialPort.h>

namespace fs = std::filesystem;
using namespace LibSerial;

// msg string -- OUT
#define LRA_USB_OUT_INIT_STR ("LAB602 LRA USB OUT init\r\n")

// msg string -- IN
#define LRA_USB_IN_INIT_STR ("LAB602 LRA USB IN init\r\n")

typedef struct {
  uint16_t VID;
  uint16_t PID;
  std::string path;
  std::string product_name;
} USB_INFO_t;

typedef enum {
  LRA_USB_OUT_INIT_DL = sizeof(LRA_USB_OUT_INIT_STR) - 1,
  LRA_USB_OUT_UPDATE_PWM_DL = 0x01,
} LRA_USB_Out_DL_t;

typedef enum {
  LRA_USB_IN_INIT_DL = sizeof(LRA_USB_IN_INIT_STR) - 1,
  LRA_USB_IN_UPDATE_PWM_DL = 0x01,
} LRA_USB_In_DL_t;

typedef enum {
  LRA_USB_CMD_INIT = 0x00,
  LRA_USB_CMD_UPDATE_PWM = 0x01,
  LRA_USB_CMD_UPDATE_REG = 0x02,  // TODO: 更新哪一個的 register 放在 data 裡面好了
} LRA_USB_Cmd_t;

// static USB_INFO_t get_target_vid_pid_usb_info(uint16_t pid, uint16_t vid);

int main() {
  spdlog::fmt_lib::print("Start to get all ttyUSB* and ttyACM* path\n");

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
    SerialPort stm32_usb;

    try {
      stm32_usb.Open(usb_tty_path[usr_input].c_str());
      stm32_usb.SetDTR(true);

      /* send init key string */
      uint8_t cmd;
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

    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  const uint16_t pid = 48602;  // DRV_STM, product name
  const uint16_t vid = 1155;   // STM

  // USB_INFO_t usb_info = get_target_vid_pid_usb_info(pid, vid);
}

// static USB_INFO_t get_target_vid_pid_usb_info(uint16_t pid, uint16_t vid) {
//   USB_INFO_t info = {
//     .VID = vid,
//     .PID = pid,
//     .path = "",
//     .product_name = ""
//   };

//   libusb_context *context = NULL;
//   libusb_init(&context);
//   libusb_device_handle *handle = libusb_open_device_with_vid_pid(context, vid, pid);
//   if (handle == NULL) {
//     std::cerr << "Device not found" << std::endl;
//     return info;
//   }

//   libusb_device_descriptor desc;
//   libusb_get_device_descriptor(libusb_get_device(handle), &desc);

//   libusb_get_string_descriptor_ascii()

// }
