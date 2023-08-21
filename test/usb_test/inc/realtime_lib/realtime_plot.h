/*
 * File: realtime_plot.h
 * Created Date: 2023-08-21
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday August 21st 2023 12:04:46 pm
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

namespace lra::realtime_plot {
bool isRunningInWSL();
std::string createPipe(std::string info);
};  // namespace lra::realtime_plot
