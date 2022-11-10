#include <main/main.h>
#include <spdlog/fmt/chrono.h>
#include <unistd.h>

#include <asio/io_service.hpp>
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
  std::shared_ptr<LogUnit> main_p = LogUnit::CreateLogUnit("main");
  std::shared_ptr<LogUnit> acc_p = LogUnit::CreateLogUnit("acc_data");
  std::shared_ptr<LogUnit> drv_p = LogUnit::CreateLogUnit("drv_data");

  // loggers pattern
  std::string system_normal = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [ %! ] %v";

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
  auto system_logger = spdlog::rotating_logger_mt("system", "/home/ubuntu/LRA/data/log/lra/system.log", rot_max_size,
                                                  rot_max_files, true, sys_handlers);
  auto acc_logger =
      spdlog::basic_logger_mt("acc_data", "/home/ubuntu/LRA/data/log/lra/acc_data.log", true, acc_handlers);
  auto drv_logger =
      spdlog::basic_logger_mt("drv_data", "/home/ubuntu/LRA/data/log/lra/drv_data.log", true, drv_handlers);

  system_logger->set_pattern(system_normal);
  system_logger->set_level(loglevel::trace);
  spdlog::set_default_logger(system_logger);

  acc_logger->set_pattern(data_json);
  acc_logger->set_level(loglevel::info);

  drv_logger->set_pattern(data_json);
  drv_logger->set_level(loglevel::info);

  spdlog::flush_every(std::chrono::seconds(1));

  // assign logger to acc_p and drv_p
  acc_p->AddLogger(acc_logger);
  drv_p->AddLogger(drv_logger);

  // create Controller -> Init -> Run
  auto controller_p = std::make_unique<lra::controller::Controller>();
  controller_p->Init();  // measure task start in another thread

  // Timer
  Timer timer;
  uint32_t event_uid = timer.SetLoopEvent(TrigLoopExpired, 10.0);

  // control loop related
  bool leave_control_loop = false;
  std::vector<uint8_t> ws_rtp_cmd{0, 0, 0};  // use vectorToTuple to covert

  // server settings and callbacks
  asio::io_service mainEventLoop;
  lra::websocket::WebsocketServer ws_server;

  ws_server.connect([&mainEventLoop, &ws_server, &main_p](ClientConnection conn) {
    mainEventLoop.post([conn, &ws_server, &main_p]() {
      main_p->LogToDefault(loglevel::info, "ws_server new connection, total: {}", ws_server.numConnections());

      // Send a hello message to the client
      // ws_server.sendMessage(conn, "hello", Json::Value());
    });
  });

  ws_server.disconnect([&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn) {
    mainEventLoop.post([conn, &ws_server, &main_p, &controller_p]() {
      main_p->LogToDefault(loglevel::info, "ws_server new connection, total: {}", ws_server.numConnections());
      if (ws_server.numConnections() == 0) {
        // reset need_send_rt
        need_send_rt = false;
        controller_p->PauseDrv();
      }
    });
  });

  /* send back module state: drv - run or stop, host name, update uuid */
  ws_server.message("moduleInfoRequire", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                              const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // get uuid of web

      // XXX no uuid
      std::string uuid_ = args["uuid"].asString();
      uuid = uuid_;

      Json::Value info;
      Json::Value data;
      char hostname[512];
      gethostname(hostname, 511);
      data["hostname"] = std::string(hostname);
      data["activated"] = controller_p->drv_x_->GetRun();

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "moduleInfoRequireResponse", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `moduleInfoRequire`, from uuid: {} ", uuid);
    });
  });

  ws_server.message("drvDriveUpdate", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                           const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      bool to_run = args["data"]["msg"].asBool();

      // XXX: set to_run, should compared to all run states
      if (to_run != controller_p->drv_x_->GetRun()) {
        to_run ? controller_p->RunDrv() : controller_p->PauseDrv();
      }

      Json::Value info;
      Json::Value data;

      data["msg"] = "ok";
      data["run"] = controller_p->drv_x_->GetRun();

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "drvDriveUpdateRecv", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `drvDriveUpdate`, to_run set to: {} ", to_run);
    });
  });

  ws_server.message("drvCmdUpdate", [&mainEventLoop, &ws_server, &main_p, &controller_p, &ws_rtp_cmd](
                                        ClientConnection conn, const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p, &ws_rtp_cmd]() {
      uint8_t rtp_x = args["data"]["x"].asUInt();
      uint8_t rtp_y = args["data"]["y"].asUInt();
      uint8_t rtp_z = args["data"]["z"].asUInt();

      ws_rtp_cmd[0] = rtp_x;
      ws_rtp_cmd[1] = rtp_y;
      ws_rtp_cmd[2] = rtp_z;

      on_update_cmd = true;

      Json::Value info;
      Json::Value data;

      data["msg"] = "ok";

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "drvCmdUpdateRecv", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `drvCmdUpdate`, new cmd x:{}, y:{}, z:{}", rtp_x, rtp_y, rtp_z);
    });
  });

  ws_server.message("regAllUpdate", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                         const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // log
      main_p->LogToDefault(loglevel::info, "ws receive `regAllUpdate starts`");

      std::vector drv_x_arr = Uint8JsonArrayToVec(args["data"]["drv"]["x"]);
      std::vector drv_y_arr = Uint8JsonArrayToVec(args["data"]["drv"]["y"]);
      std::vector drv_z_arr = Uint8JsonArrayToVec(args["data"]["drv"]["z"]);
      std::vector acc_arr = Uint8JsonArrayToVec(args["data"]["acc"]);  // spi 有 rw lock

      on_modify = true;
      controller_p->adxl_->SetStandBy(true);
      /* write to  */
      controller_p->UpdateAllRegisters(std::make_tuple(drv_x_arr, drv_y_arr, drv_z_arr, acc_arr));
      if (!on_calibration) {
        controller_p->adxl_->SetStandBy(false);  // XXX: 可能有誤提
      }

      on_modify = false;

      Json::Value info;
      Json::Value data;

      data["msg"] = "ok";

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "regAllUpdateRecv", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `regAllUpdate` finished, size -- x: {}, y: {}, z: {}, acc: {}",
                           drv_x_arr.size(), drv_y_arr.size(), drv_z_arr.size(), acc_arr.size());
    });
  });

  ws_server.message("calibrationRequire", [&mainEventLoop, &ws_server, &main_p, &controller_p](
                                              ClientConnection conn, const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // log
      auto start = std::chrono::system_clock::now();
      main_p->LogToDefault(loglevel::info, "ws receive calibrationRequire");

      on_calibration = true;
      controller_p->adxl_->SetStandBy(true);
      auto cal_result = controller_p->RunCalibration();
      controller_p->adxl_->SetStandBy(false);
      on_calibration = false;

      /* TODO: print local */

      /* write to json */
      auto [drv, acc] = CalibrationResultToJson(cal_result);

      Json::Value info;
      Json::Value data;

      data["msg"] = "ok";
      data["drv"] = drv;
      data["acc"] = acc;

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "calibrationRequireResponse", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `calibrationRequire` finished, cost: {:.4f}(s)",
                           (now - start).count() / 1e9);
    });
  });

  ws_server.message("regAllRequire", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                          const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // log

      auto [v_x, v_y, v_z, v_acc_ro, v_acc_rw] = controller_p->GetAllRegisters();  // seperate by FIFO

      /* TODO: print local */

      /* write to json */

      Json::Value info;
      Json::Value data;
      Json::Value drv;
      Json::Value acc;

      drv["x"] = VecToJson(v_x);
      drv["y"] = VecToJson(v_y);
      drv["z"] = VecToJson(v_z);
      acc["ro"] = VecToJson(v_acc_ro);
      acc["rw"] = VecToJson(v_acc_rw);

      data["msg"] = "ok";
      data["drv"] = drv;
      data["acc"] = acc;

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // Echo the message pack to the client
      ws_server.sendMessage(conn, "regAllRequireResponse", info);

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `regAllRequire`");
    });
  });

  ws_server.message("dataRTKeepRequire", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                              const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // XXX
      need_send_rt = true;

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `dataRTKeepRequire`, start to send real time data");
    });
  });

  ws_server.message("dataRTStopRequire", [&mainEventLoop, &ws_server, &main_p, &controller_p](ClientConnection conn,
                                                                                              const Json::Value &args) {
    mainEventLoop.post([conn, args, &ws_server, &main_p, &controller_p]() {
      // XXX
      need_send_rt = false;

      Json::Value info;
      Json::Value data;

      data["msg"] = "ok";

      auto now = std::chrono::system_clock::now();
      info["uuid"] = uuid;
      info["timestamp"] = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());
      info["data"] = data;

      // log
      main_p->LogToDefault(loglevel::info, "ws receive `dataRTStopRequire`, stop to send real time data");
    });
  });

  // create control loop thread
  std::thread controller_t = std::thread([&controller_p, &leave_control_loop, &ws_rtp_cmd, &main_p, &ws_server]() {
    int i = 0;

    /* debug */

    while (!leave_control_loop) {
      if (control_loop_expired && !on_calibration) {
        control_loop_expired = false;
        // test
        // auto now = std::chrono::system_clock::now();
        // main_p->LogToDefault(loglevel::info, "t: {0:%Y-%m-%d %H:%M:}{1:%S}", now, now.time_since_epoch());

        if (!on_modify) {  // 沒收到 webpage 的更改指令，安全嗎? mutex
          // if (on_run) {
            controller_p->RunDrv();  // 確保有在運作

            /* set rtp by my self debug */
            ws_rtp_cmd[0] = 0xff;
            ws_rtp_cmd[1] = 0xff;
            ws_rtp_cmd[2] = 0xff;
            controller_p->UpdateAllRtp(VecToTuple<3, uint8_t>(ws_rtp_cmd));
          // }

          if (on_update_cmd) {
            controller_p->UpdateAllRtp(VecToTuple<3, uint8_t>(ws_rtp_cmd));
            on_update_cmd = false;
          }

          // XXX: only allows one client and broadcast mode
          if (need_send_rt) {
            /****************************** write to web *****************************/

            // get real time info
            auto now = std::chrono::system_clock::now();
            auto [rt_x, rt_y, rt_z] = controller_p->GetRt();  // if 0 might be wiring problem
            auto v = controller_p->adxl_->AccPopAll();        // XXX: should be tune with PopN

            // XXX rewrite this

            Json::Value payload;
            Json::Value data;
            Json::Value drv;
            Json::Value acc;

            Json::Value drv_1axis;

            /* time */
            drv["t"] = (now - controller_p->start_time_).count();

            /* drv */
            drv_1axis["rtp"] = rt_x.rtp_;
            drv_1axis["freq"] = rt_x.lra_freq_;

            drv["x"] = drv_1axis;

            drv_1axis["rtp"] = rt_y.rtp_;
            drv_1axis["freq"] = rt_y.lra_freq_;

            drv["y"] = drv_1axis;

            drv_1axis["rtp"] = rt_z.rtp_;
            drv_1axis["freq"] = rt_z.lra_freq_;

            drv["z"] = drv_1axis;

            /* acc */
            for (auto &e : v) {
              acc.append(Acc3ToJson(e));
            }

            data["drv"] = drv;
            data["acc"] = acc;

            // move to thread if cost to much time
            std::string timestamp = spdlog::fmt_lib::format("{:%Y-%m-%d %H:%M:}{:%S}", now, now.time_since_epoch());

            payload["uuid"] = uuid;
            payload["timestamp"] = timestamp;
            payload["data"] = data;

            /*  XXX: can't get conn, so use broadcast*/
            ws_server.broadcastMessage("dataRTKeepRequireResponse", payload);

            /****************************** write to local *****************************/
            // TODO
          }

        } else {
          controller_p->PauseDrv();  // ensure module is not driven
        }

        // DEBUG: alive log
        i++;
        if (i >= 100) {
          i = 0;
          /* get mode --DEBUG */
          controller_p->ChangeDrvCh('x');
          auto now = std::chrono::system_clock::now();
          main_p->LogToDefault(loglevel::debug, "controller thread alive: : {:%Y-%m-%d %H:%M:}{:%S}" , now,
                               now.time_since_epoch());
        }
      } else {
        std::this_thread::yield();
      }
    }

    return;
  });

  main_p->LogToDefault(loglevel::info, "ws server on");

	//Start the networking thread
	std::thread serverThread([&ws_server]() {
		ws_server.run(8787);
	});

  main_p->LogToDefault(loglevel::info, "mainEventLoop on");
  system_logger->flush();

  // blocks here
  asio::io_service::work work(mainEventLoop);
  mainEventLoop.run();

  /* should never goes to here */

  // if server down
  leave_control_loop = true;

  // drop controller
  controller_p->CancelMeasureTask();
  controller_p.reset();

  // drop loop event
  timer.CancelEvent(event_uid);

  // drop loggers
  spdlog::drop_all();

  return 0;
}

// functions impl
void TrigLoopExpired() { control_loop_expired = true; }

std::tuple<Json::Value, Json::Value> CalibrationResultToJson(
    const std::tuple<Drv2605lInfo, Drv2605lInfo, Drv2605lInfo, Adxl355::Acc3> &t) {
  auto [s_x, s_y, s_z, s_acc] = t;

  Json::Value drv_x = Drv2605lInfoToJson(s_x);
  Json::Value drv_y = Drv2605lInfoToJson(s_y);
  Json::Value drv_z = Drv2605lInfoToJson(s_z);
  Json::Value acc = Acc3ToJson(s_acc);
  Json::Value drv;

  drv["x"] = drv_x;
  drv["y"] = drv_y;
  drv["z"] = drv_z;

  return std::make_tuple(drv, acc);
}

Json::Value Drv2605lInfoToJson(const Drv2605lInfo &data) {
  Json::Value result;
  result["id"] = data.device_id_;
  result["result"] = data.diag_result_;
  result["vbat"] = data.vbat_;
  result["freq"] = data.lra_freq_;
  result["bemf"] = data.back_emf_result_;
  result["fb_coeff"] = data.compensation_coeff_;
  return result;
}

Json::Value Acc3ToJson(const Adxl355::Acc3 &data) {
  Json::Value result;
  result["t"] = data.time;
  result["x"] = data.data.x;
  result["y"] = data.data.y;
  result["z"] = data.data.z;
  return result;
}

Json::Value VecToJson(const std::vector<uint8_t> v) {
  Json::Value result;
  for (auto &e : v) {
    result.append(e);
  }
  return result;
}

std::vector<uint8_t> Uint8JsonArrayToVec(const Json::Value &arr) {
  std::vector<uint8_t> v;
  v.reserve(arr.size());

  for (auto &e : arr) {
    v.push_back(e.asUInt());
  }

  return v;
}
