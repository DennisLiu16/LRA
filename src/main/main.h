#ifndef LRA_MAIN_H_
#define LRA_MAIN_H_

#include <controller/controller.h>
#include <util/log/logunit.h>
#include <util/timer/timer.h>
#include <websocket/websocket.h>

/* spdlog */
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

constexpr auto rot_max_size = 1048576 * 5;  // 5 MB
constexpr auto rot_max_files = 3;

/* using */
using ::lra::device::Adxl355;
using ::lra::device::Drv2605lInfo;
using ::lra::log_util::loglevel;
using ::lra::log_util::LogUnit;
using ::lra::timer_util::Timer;
using ::lra::websocket::ClientConnection;

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
    "}},"};

std::string drv_format_str = {
    "{{\"t\": \"{:.4f}\", "
    "\"x\": " +
    drv_1axis_str + "\"y\": " + drv_1axis_str + "\"z\": " + drv_1axis_str + "}},"};

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
bool control_loop_expired{false};
bool on_calibration{false};
bool on_modify{false};
bool on_update_cmd{false};
bool on_run{false};

/* FIXME: should not be global */
bool need_send_rt{false};
std::string uuid{""};

/* functions */
void TrigLoopExpired();

Json::Value Drv2605lInfoToJson(const Drv2605lInfo& data);

Json::Value Acc3ToJson(const Adxl355::Acc3& data);

Json::Value VecToJson(const std::vector<uint8_t> v);

std::tuple<Json::Value, Json::Value> CalibrationResultToJson(
    const std::tuple<Drv2605lInfo, Drv2605lInfo, Drv2605lInfo, Adxl355::Acc3>& t);

std::vector<uint8_t> Uint8JsonArrayToVec(const Json::Value& arr);

template <typename T, std::size_t... Indices>
auto vectorToTupleHelper(const std::vector<T>& v, std::index_sequence<Indices...>) {
  return std::make_tuple(v[Indices]...);
}

template <std::size_t N, typename T>
auto VecToTuple(const std::vector<T>& v) {
  assert(v.size() >= N);
  return vectorToTupleHelper(v, std::make_index_sequence<N>());
}

#endif