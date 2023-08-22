/*
 * File: realtime_plot.h
 * Created Date: 2023-08-21
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday August 21st 2023 5:21:05 pm
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

#include <string>

namespace lra::realtime_plot {
bool isRunningInWSL();
bool createPipe(const std::string& pipe_path);
bool ensureDirectoryExists(const std::string& dirPath);
};  // namespace lra::realtime_plot
