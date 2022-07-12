#include <sys/time.h>
#include <util/timer/timer.h>

// #include <array>
// #include <algorithm>
// #include <chrono>
#include <cmath>
// #include <functional>
// #include <queue>

#include <util/log/log.h>

namespace lra::timer_util {

namespace chrono = std::chrono;

Timer::Timer() {
  nanosleep_delay_us_ = static_cast<uint32_t>(Value::kDefaultDelay);
  // open a thread for run (background)
  std::function<void()> daemon = std::bind(&Timer::Run, this, std::thread::hardware_concurrency() / 2);
  logunit = lra::log_util::LogUnit::CreateLogUnit(*this);
  logunit->Log(spdlog::level::debug, "Timer daemon started");
  std::thread t1(daemon);
  t1.detach();
}

Timer::Timer(uint32_t thread_num, DelayOpt opt = DelayOpt::kDefaultDelay) {
  if (opt == DelayOpt::kDefaultDelay) {
    nanosleep_delay_us_ = static_cast<uint32_t>(Value::kDefaultDelay);
  }

  if (nanosleep_delay_us_ == 0) {  // get nanosleep average delay ~ thread::sleep_for
    nanosleep_delay_us_ = GetErrorOfNanosleep();
  }

  // open a thread for run (background)
  std::function<void()> daemon = std::bind(&Timer::Run, this, thread_num);
  logunit = lra::log_util::LogUnit::CreateLogUnit();
  std::thread t1(daemon);
  t1.detach();
}

Timer::~Timer() {
  run_flag_ = false;
  while (!can_destroy_) {
    ;
  }
}

template <typename F>
uint32_t Timer::SetEvent(const F& task, double duration_ms) {
  TimerEvent te = CreateTimerEvent(task, duration_ms);

  event_queue_.push(te);
  push_flag_ = true;

  return te.uid;
}

template <typename F>
uint32_t Timer::SetLoopEvent(const F& task, double period_ms) {
  TimerEvent te = CreateTimerEvent(task, period_ms);
  te.is_loop_event = true;

  event_queue_.push(te);
  push_flag_ = true;
  return te.uid;
}

// event will be removed at next pop
bool Timer::CancelEvent(uint32_t uid) {
  if (uid >= uid_for_next_event_) {
    logunit->Log(spdlog::level::warn, "uid: {} invalid - exceeds uid_for_next_event_", uid);
    return false;
  }
  bool ret = RemoveUid(uid);
  if (!ret) {
    logunit->Log(spdlog::level::warn, "this uid: {} was already been removed", uid);
  }
  return ret;
}

// delete the timer instance
void Timer::Delete() {
  // stop subthread first, or can_destroy_ will lead segmentation fault
  run_flag_ = false;
  while (!can_destroy_) {
  }
}

// return interrupted or not
bool Timer::PreciseSleepms(double ms, bool enable_interrupted_by_new_event = 0) {
  if (ms < 0.0) return false;

  // critical value for raspberry 4b
  bool larger_than_7ms = (ms >= 7.0);
  static double mean, estimate = (larger_than_7ms) ? 5 : 0.5;
  mean = estimate;
  static double m2 = 0;
  static uint64_t count = 1;
  auto sleep_period = (larger_than_7ms) ? chrono::milliseconds(1) : chrono::microseconds(nanosleep_delay_us_);
  double X = (larger_than_7ms) ? 1 : 2;

  while (ms > estimate) {                                 // thread sleep
    if (enable_interrupted_by_new_event && push_flag_) {  // leave because of event push interrupting
      push_flag_ = false;
      return true;
    }
    auto start = chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(sleep_period);
    auto end = chrono::high_resolution_clock::now();

    double observed = (end - start).count() / 1e6;  // to ms
    ms -= observed;

    count++;  // put here for below calculationf

    double delta = observed - mean;
    mean += delta / count;
    m2 += delta * (observed - mean);
    double stddev = sqrt(m2 / (count - 1));
    estimate = mean + X * stddev;
  }

  // spin lock
  auto start = chrono::high_resolution_clock::now();
  while ((chrono::high_resolution_clock::now() - start).count() / 1e6 < ms) {
    if (enable_interrupted_by_new_event && push_flag_) {
      push_flag_ = false;
      return true;
    }
  };
  return false;
}

// private

// create a bcakground thread to monitor event_queue_
void Timer::Run(uint32_t thread_num) {
  // make thread pool
  BS::thread_pool pool(thread_num);

  t_now_ = chrono::high_resolution_clock::now();

  // main
  while (run_flag_) {
    static double sleep_time_ms = 0.0;

    if (event_queue_.empty()) {  // if event_queue_ is empty, sleep 1 sec
      sleep_time_ms = static_cast<double>(Value::kIdleSleepMs);
    } else {  // expired check(not empty), do task if poll ok, sort the events, calculate next closest interval

      HandleExpiredEvents(pool);
      sleep_time_ms = EvalNextInterval();  // update t_now_ here too
    }

    PreciseSleepms(sleep_time_ms, true);  // can be interrupted
    t_now_ = chrono::high_resolution_clock::now();
  }

  can_destroy_ = true;
}

template <typename F>
TimerEvent Timer::CreateTimerEvent(const F& task, double period_ms) {
  TimerEvent te;
  te.is_loop_event = false;
  te.period_ms = period_ms;
  te.task = task;
  // TODO: Add a mutex for valid_uid_
  valid_uid_.push_back(uid_for_next_event_);  // register uid
  te.uid = uid_for_next_event_++;             // increment 1 after assign to te.uid
  te.t = std::chrono::high_resolution_clock::now();
  if (period_ms < 1.0) {
    logunit->Log(spdlog::level::warn, "uid: {}'s period is {:.4f} < 1.0 (ms), timer may crushed", te.uid, period_ms);
  }
  return te;
}

// From valid_uid_ remove target uid
bool Timer::RemoveUid(uint32_t uid) {
  // TODO: Add a mutex here to lock
  // uid in valid_uid_ 100% < uid_for_next_event_

  // find and pop
  auto it = std::find(valid_uid_.begin(), valid_uid_.end(), uid);
  if (valid_uid_.end() == it) {  // this uid was removed already
    return false;
  }

  // TODO: add a mutex here
  valid_uid_.erase(it);
  return true;
}

// use gettimeofday() to evaluate delay(us) in nanosleep
// pass unit test
uint32_t Timer::GetErrorOfNanosleep() {
  constexpr std::array<uint32_t, 20> delay = {500000, 100000, 50000, 10000, 1000, 900, 500, 100, 10, 1};

  timespec req;
  timeval tval_begin, tval_end;
  uint32_t t_diff = 0;

  for (auto& it : delay) {
    req.tv_sec = it / 1000000;
    // ref : https://www.zhihu.com/question/22747596
    // general optimization of mod: a%b ~ (a - a/b * b) if b is a constant
    req.tv_nsec = (it - it / 1000000 * 1000000) * 1000;

    gettimeofday(&tval_begin, nullptr);
    int ret = nanosleep(&req, nullptr);
    if (-1 == ret) {
      logunit->Log(spdlog::level::err, "nanosleep doesn't support in this OS, return delay to default value: {} (us)",
                                   static_cast<uint32_t>(Value::kDefaultDelay));
      return static_cast<uint32_t>(Value::kDefaultDelay);
    }
    gettimeofday(&tval_end, nullptr);
    t_diff += (tval_end.tv_sec - tval_begin.tv_sec) * 1000000 + tval_end.tv_usec - tval_begin.tv_usec - it;
  }

  t_diff /= delay.size();
  logunit->Log(spdlog::level::info, "nanosleep delay: {} (us)", t_diff);

  return t_diff;
}

double Timer::GetErrorOfTimerMs(double duration_ms) {
  // TODO:shared_ptr?;
  return 0.0;
}

void Timer::HandleExpiredEvents(BS::thread_pool& pool) {  // events should be well sorted in queue

  if (event_queue_.empty()) return;

  static uint64_t overloading_count = 0;

  // construct a tmp queue to store popped loop events
  std::queue<TimerEvent> popped_loop_events_queue;

  do {  // assign expired tasks to thread pool, and store loop events to tmp queue
    TimerEvent event = event_queue_.top();

    if (!IsValidUid(event.uid)) {  // top is not valid event anymore, pop
      event_queue_.pop();
      continue;
    }

    if (!(IsNearExpired(event))) {  // leave this loop when no event expired
      break;
    }

    if (event.is_loop_event) {
      popped_loop_events_queue.push(event);
    }

    if (!HasIdleThread(pool)) {  // overloading record
      overloading_count++;
      if ((overloading_count - overloading_count / 10 * 10)) {  // send debug log every ten times
        logunit->Log(spdlog::level::warn, "Timer Overloading: {}", overloading_count);
      }
    }

    // push_task to thread pool
    pool.push_task(event.task);

    event_queue_.pop();
  } while (!event_queue_.empty());

  t_now_ = chrono::high_resolution_clock::now();

  // push tmp queue elements to event_queue_
  while (!popped_loop_events_queue.empty()) {
    // update time
    TimerEvent event = popped_loop_events_queue.front();
    event.t = chrono::high_resolution_clock::now();

    event_queue_.push(event);
    popped_loop_events_queue.pop();
  }
}

double Timer::EvalNextInterval() {
  if (event_queue_.empty()) {
    return static_cast<double>(Value::kIdleSleepMs);
  }

  auto event = event_queue_.top();
  return event.period_ms - EvalTimeDiffFromNow(event.t);
}

double Timer::EvalTimeDiffFromNow(chrono::system_clock::time_point& t) {
  t_now_ = chrono::high_resolution_clock::now();
  return (t_now_ - t).count() / 1e6;
}

// you should notice that
// - no const keyword
// - if you don't know how to write, compiler will tell you
// e.g. undefined reference to `unsigned int lra::timer_util::Timer::SetLoopEvent<void ()>(void ( const&)(), double)'
// --> template uint32_t Timer::SetLoopEvent<void ()>(void(&task)(), double);
template uint32_t Timer::SetEvent<void()>(void (&task)(), double);
template uint32_t Timer::SetLoopEvent<void()>(void (&task)(), double);
template TimerEvent Timer::CreateTimerEvent<void()>(void (&task)(), double);
};  // namespace lra::timer_util
