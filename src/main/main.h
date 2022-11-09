#ifndef LRA_MAIN_H_
#define LRA_MAIN_H_

#include <controller/controller.h>
#include <util/log/logunit.h>
#include <util/timer/timer.h>

/* spdlog */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

constexpr auto rot_max_size = 1048576 * 5;  // 5 MB
constexpr auto rot_max_files = 3;

/* using */
using ::lra::log_util::loglevel;
using ::lra::log_util::LogUnit;
using ::lra::timer_util::Timer;

/* format string */
std::string acc_format_str = {
    "{{\"t\": \"{:.4f}\", "
    "\"x\": \"{:.4f}\", "
    "\"y\": \"{:.4f}\", "
    "\"z\": \"{:.4f}\"}},"};

std::string drv_1axis_str = {
  "{{"
  "\"cmd\": \"{:d}\", "
  "\"freq\": \"{:.3f}\""
  "}},"
};

std::string drv_format_str = {
    "{{\"t\": \"{:.4f}\", "
    "\"x\": " + drv_1axis_str +
    "\"y\": " + drv_1axis_str +
    "\"z\": " + drv_1axis_str +
    "}},"};

// fprintf format
const char* syslog_fformat_onopen = 
    "\n\n"                          
    "**************************\n"  
    "* LRA SYSTEM LOGGER OPEN *\n"  
    "**************************\n"  
    " start time: %s\n\n\n"          
    "";

const char* acclog_fformat_onopen = 
    "{"
    "acc: ["
    "";

const char* drvlog_fformat_onopen =
    "{"
    "drv: ["
    "";
const char* datalog_fformat_onclose = "],}";

/* global states */
bool loop_expired{false};

/* functions */
void TrigLoopExpired();



#endif