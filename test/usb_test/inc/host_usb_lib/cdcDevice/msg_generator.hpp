/*
 * File: msg_generator.hpp
 * Created Date: 2023-04-05
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday April 10th 2023 4:56:32 pm
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

#include <drv_stm_lib/lra_usb_defines.h>

#include <cstdint>

namespace lra::usb_lib {
class RCWSMsgGenerator {
 public:
  std::vector<uint8_t> Generate(uint8_t type, uint8_t* data, uint16_t length) {
    if (data == nullptr) return {};

    std::vector<uint8_t> vec(data, data + length);
    return Generate(type, vec);
  }

  std::vector<uint8_t> Generate(uint8_t type, uint8_t val) {
    std::vector<uint8_t> vec{val};
    return Generate(type, vec);
  }

  /**
   * There is no need to impl generate(uint8_t type, std::vector<uint8_t>&&
   * data). It's because there's no huge size data need to be copied.
   *
   * Note that this function will add \r\n at the end. Therefore, data should
   * not include \r\n
   */
  std::vector<uint8_t> Generate(uint8_t type, std::vector<uint8_t> data = {}) {
    uint16_t len = data.size() + 2;
    std::vector<uint8_t> ret_vec{type, (uint8_t)(len >> 8), (uint8_t)len};
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
