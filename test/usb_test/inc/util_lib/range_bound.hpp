/*
 * File: range_bound.hpp
 * Created Date: 2023-05-22
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday May 23rd 2023 1:41:16 pm
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

namespace lra::util {
class Range {
 public:
  Range(float min, float max) : min_(min), max_(max) {}

  bool isWithinRange(float value) const { return value > min_ && value < max_; }

 private:
  float min_;
  float max_;
};
}  // namespace lra::util
