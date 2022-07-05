#ifndef LRA_UTIL_TIMER_H_
#define LRA_UTIL_TIMER_H_

#include <util/log/log.h>
#include <thread-pool/BS_thread_pool.hpp>

namespace lra::timer_util {

// namespace of util/timer

struct TimerEvent {
  bool is_loop_event;
  int uid;
  double period_ms;
  std::chrono::system_clock::time_point t;
  std::function<void()> task;  // TODO: enable non-void function

  bool operator<(const TimerEvent& rhs) const {  // t + delta_t < rhs.t + rhs.delta_t --> (t-rhs.t) < rhs.delta_t - delta_t;
    // note that the priority queue order from larger to smaller
    // so we reverse to (t-rhs.t) > rhs.delta_t - delta_t;
    // make high freq tasks high priority
    return ((t - rhs.t).count() / 1e6) > (rhs.period_ms - period_ms);
  }
};

// Movable but not copyable class
// Can do precise sleep and schedule task by
class Timer {
 public:
  // enum of init args
  enum class DelayOpt { kMeasureDelay,
                        kDefaultDelay };
  enum class Value { kDefaultDelay = 70,
                     kIdleSleepMs = 1000 };

  std::shared_ptr<lra_log_util::LogUnit> logunit;

  // constructor
  Timer();
  explicit Timer(uint32_t thread_num, DelayOpt opt);
  Timer(Timer&& movable_timer) = default;
  Timer(const Timer& copyable_timer) = delete;

  // assign operator
  Timer& operator=(Timer&& movable_timer) = default;
  Timer& operator=(const Timer& copyable_timer) = delete;

  // destructor
  ~Timer();

  template <typename F>
  uint32_t SetEvent(const F& task, double duration_ms);

  template <typename F>
  uint32_t SetLoopEvent(const F& task, double period_ms);

  bool CancelEvent(uint32_t uid);

  void Delete();

  // return interrupted or not
  bool PreciseSleepms(double ms, bool enable_interrupted_by_new_event);

 private:
  static constexpr double min_valid_interval_ = 0.01;  // 10 us

  uint32_t nanosleep_delay_us_ = 0;
  volatile bool run_flag_ = true;
  volatile bool can_destroy_ = false;
  volatile bool push_flag_ = false;
  std::chrono::system_clock::time_point t_now_;
  uint32_t uid_for_next_event_ = 0;
  // issue 1: share private member among different threads
  // ref: https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#Test
  // issue 2: reference type in STL
  // ref: https://hackmd.io/h7i1FlqvS9iIh8FU0usJZQ#Constructor-speed-of-STL-vector-on-Pi4b

  std::priority_queue<TimerEvent> event_queue_;
  std::vector<uint32_t> valid_uid_;

  void Run(uint32_t thread_num);
  bool RemoveUid(uint32_t uid);
  uint32_t GetErrorOfNanosleep();
  double GetErrorOfTimerMs(double duration_ms);

  // template create a Timerevent and return
  template <typename F>
  TimerEvent CreateTimerEvent(const F& task, double period_ms);

  void HandleExpiredEvents(BS::thread_pool& pool);
  double EvalNextInterval();
  double EvalTimeDiffFromNow(std::chrono::system_clock::time_point& t);

  // @aborted
  inline bool IsExpired(TimerEvent& event) {
    return (EvalTimeDiffFromNow(event.t) > event.period_ms);
  }

  inline bool IsNearExpired(TimerEvent& event) {
    return (EvalTimeDiffFromNow(event.t) > (event.period_ms + min_valid_interval_));
  }

  inline bool HasIdleThread(BS::thread_pool& pool) {
    return (pool.get_thread_count() > pool.get_tasks_running());
  };

  inline bool IsValidUid(uint32_t uid) {  // note that uid vector should be sorted (always push_back)
    return std::binary_search(valid_uid_.begin(), valid_uid_.end(), uid);
  };
};

}  // namespace lra::timer_util
#endif
