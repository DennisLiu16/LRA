/*
 * File: msg_generator.hpp
 * Created Date: 2023-04-05
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday May 2nd 2023 4:07:25 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */
#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <drv_stm_lib/lra_usb_defines.hpp>

namespace lra::usb_lib {

class RcwsMsgGenerator {
 public:
  std::vector<uint8_t> Generate(LRA_USB_OUT_Cmd_t type, uint8_t* data,
                                uint16_t length) {
    if (data == nullptr) return {};

    std::vector<uint8_t> vec(data, data + length);
    return Generate(type, vec);
  }

  std::vector<uint8_t> Generate(LRA_USB_OUT_Cmd_t type, uint8_t data) {
    std::vector<uint8_t> vec{data};
    return Generate(type, vec);
  }

  /**
   *  const msg type generate
   **/

  std::vector<uint8_t> Generate(LRA_USB_OUT_Cmd_t type) {
    switch (type) {
      case USB_OUT_CMD_INIT: {
        std::string s = rcws_msg_init;
        uint16_t len = s.length();
        std::vector<uint8_t> vec{(uint8_t)type, (uint8_t)(len >> 8),
                                 (uint8_t)len};
        vec.insert(vec.end(), s.begin(), s.end());
        return vec;
      }

      /* some msg only include \r\n msg */
      // return Generate(type, {});
      default:
        /* TODO: you should never get here */
        assert(false &&
               "LRA_USB_OUT_Cmd_t is not const msg type. Please use "
               "other Generate function includeing param: 'data'");
    }
  }

  /**
   * There is no need to impl generate(uint8_t type, std::vector<uint8_t>&&
   * data). It's because there's no huge size data need to be copied.
   *
   * Note that this function will add \r\n at the end. Therefore, data should
   * not include \r\n
   */
  std::vector<uint8_t> Generate(LRA_USB_OUT_Cmd_t type,
                                std::vector<uint8_t> data) {
    uint16_t len = data.size() + 2;
    std::vector<uint8_t> ret_vec{(uint8_t)type, (uint8_t)(len >> 8),
                                 (uint8_t)len};
    ret_vec.insert(ret_vec.end(), data.begin(), data.end());
    // add \r\n at the end of message
    ret_vec.push_back(0x0D);
    ret_vec.push_back(0x0A);

    return ret_vec;
  }

  std::vector<uint8_t> MoveGenerate(uint8_t type, std::vector<uint8_t>&& data) {
    /* no need to impl */
    return {};
  }
};
}  // namespace lra::usb_lib
