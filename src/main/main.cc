#include <main/main.h>

#include <chrono>
#include <thread>

/**
 * @brief
 *
 * @record
 * 1. use fmt_lib runtime to generate string type format string
 *
 * @param argc
 * @param argv
 * @return int
 */

int main(int argc, char *argv[]) {
  // create logunit settings
  std::shared_ptr<LogUnit> main_p;
  std::shared_ptr<LogUnit> acc_p = LogUnit::CreateLogUnit("acc_data");
  std::shared_ptr<LogUnit> drv_p = LogUnit::CreateLogUnit("drv_data");

  // loggers pattern
  std::string system_json = {
      "{\"time\": \"%Y-%m-%dT%H:%M:%S.%f%z\", \"name\": \"%n\", \"level\": \"%^%l%$\", \"process\": %P, \"thread\": "
      "%t, \"src\": \"%s:%#\", \"logunit\": \"%!\", \"message\": \"%v\"},"};

  std::string data_json = {"%v"};

  // generate runtime format string var, in main.h
  auto acc_format = spdlog::fmt_lib::runtime(acc_format_str);
  auto drv_format = spdlog::fmt_lib::runtime(drv_format_str);

  // loggers on open, on close
  spdlog::file_event_handlers sys_handlers;
  spdlog::file_event_handlers acc_handlers;
  spdlog::file_event_handlers drv_handlers;

  // callbacks
  sys_handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) {
    auto now = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(now);

    fprintf(fstream, syslog_fformat_onopen, ctime(&t));
  };

  acc_handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) {
    fprintf(fstream, "%s", acclog_fformat_onopen);
  };

  acc_handlers.before_close = [](spdlog::filename_t filename, std::FILE *fstream) {
    fprintf(fstream, "%s", datalog_fformat_onclose);
  };

  drv_handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) {
    fprintf(fstream, "%s", drvlog_fformat_onopen);
  };

  drv_handlers.before_close = [](spdlog::filename_t filename, std::FILE *fstream) {
    fprintf(fstream, "%s", datalog_fformat_onclose);
  };

  // loggers
  auto system_logger = spdlog::rotating_logger_mt("system", "/home/ubuntu/LRA/data/log/system.log", rot_max_size,
                                                  rot_max_files, true, sys_handlers);
  auto acc_logger = spdlog::basic_logger_mt("acc_data", "/home/ubuntu/LRA/data/log/acc_data.log", true, acc_handlers);
  auto drv_logger = spdlog::basic_logger_mt("drv_data", "/home/ubuntu/LRA/data/log/drv_data.log", true, drv_handlers);

  system_logger->set_pattern(system_json);
  system_logger->set_level(loglevel::trace);
  spdlog::set_default_logger(system_logger);

  acc_logger->set_pattern(data_json);
  acc_logger->set_level(loglevel::info);

  drv_logger->set_pattern(data_json);
  drv_logger->set_level(loglevel::info);

  // assign logger to acc_p and drv_p
  acc_p->AddLogger(acc_logger);
  drv_p->AddLogger(drv_logger);

  // create Controller -> Init -> Run
  auto controller_p = std::make_unique<lra::controller::Controller>();
  controller_p->Init();  // measure task start in another thread
  controller_p->Run();

  bool leave_main = false;

  // create server

  // not on calibration or onModify

  // server -> modify wrapper

  // on_calibration
  // on_modify

  // Timer
  Timer timer;
  uint32_t event_uid = timer.SetLoopEvent(TrigLoopExpired, 10.0);

  // main loop
  while (!leave_main) {
    if (loop_expired) {
      loop_expired = false;
    } else {
      std::this_thread::yield();
    }
  }

  // drop controller
  controller_p->CancelMeasureTask();
  controller_p.reset();

  // drop loop event
  timer.CancelEvent(event_uid);

  // drop loggers
  spdlog::drop_all();
}

// functions impl
void TrigLoopExpired() { loop_expired = true; }