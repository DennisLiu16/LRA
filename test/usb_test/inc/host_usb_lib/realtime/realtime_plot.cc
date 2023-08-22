/*
 * File: realtime.cc
 * Created Date: 2023-08-21
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday August 21st 2023 5:20:11 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#include "realtime_plot.h"

#include <fcntl.h>
#include <host_usb_lib/logger/logger.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>

namespace lra::realtime_plot {
bool isRunningInWSL() {
  std::ifstream in("/proc/version");
  std::string content;
  getline(in, content);
  in.close();
  return content.find("Microsoft") != std::string::npos ||
         content.find("WSL") != std::string::npos;
}

bool createPipe(const std::string& pipe_path) {
  // Check if the named pipe already exists
  struct stat info;
  if (stat(pipe_path.c_str(), &info) == 0) {
    // If it exists and is a named pipe, try to delete it
    if (info.st_mode & S_IFIFO) {
      if (unlink(pipe_path.c_str()) == -1) {
        // Handle error if needed
        return false;
      }
    } else {
      // The path exists but is not a named pipe
      return false;
    }
  }

  // Create the named pipe
  if (mkfifo(pipe_path.c_str(), 0666) == -1) {
    return false;
  }

  return true;
}

bool ensureDirectoryExists(const std::string& dirPath) {
  struct stat info;

  // Check if the directory exists
  if (stat(dirPath.c_str(), &info) != 0) {
    // Attempt to create the directory
    if (mkdir(dirPath.c_str(), 0777) == -1) {
      return false;
    }
  } else if (!(info.st_mode & S_IFDIR)) {
    return false;
  }

  return true;
}

};  // namespace lra::realtime_plot