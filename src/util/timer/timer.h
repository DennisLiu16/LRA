#ifndef LRA_UTIL_TIMER_H_
#define LRA_UTIL_TIMER_H_

#include <time.h>

#include <chrono>
#include <functional>
#include <priority_queue>


#include <third_party/thread-pool/BS_thread_pool.hpp>

namespace lra::timer_util{

// namespace of util/timer

// The struct of member in 
struct TimerEvent {
  bool is_loop_event;
  int uid;
  double period_ms;
  std::chrono::system_clock::time_point t;
  std::function<void()> task;

  bool operator<(const TimerEvent& rhs) const
  { // t + delta_t < rhs.t + rhs.delta_t --> (t-rhs.t) < rhs.delta_t - delta_t;
    return (t - rhs.t).count()/1e6 < rhs.period_ms - period_ms;
  }

  // TODO:delete operator
};

class Timer {
  public:

    // TODO: copy constructor

    // enum of init args
    enum class DelayOpt {kMeasureDelay, kDefaultDelay};

    // operator

    Timer(uint32_t thread_num, DelayOpt opt);
    ~Timer();

    int SetEvent(std::function<void()> task, double duration_ms);

    int SetLoopEvent(std::function<void()> task, double period_ms);

    bool CancelEvent(int uid);

    void Delete();

    // return interrupted or not
    bool PreciseSleepms(double ms, bool enable_interrupted_by_new_event);
  
  private:
    static constexpr double min_valid_interval_ = 0.01; // 10 us

    uint32_t nanosleep_delay_us_ = 0;
    volatile bool run_flag_ = true;
    volatile bool push_flag_ = false;
    std::chrono::system_clock::time_point t_now_;
    int uid_for_next_event_ = 0;
    // issue 1: share private member among different threads
    // ref: https://hackmd.io/gAsWsT1RSUulPcq4de_HGQ#Test
    // issue 2: reference type in STL
    // ref: https://hackmd.io/h7i1FlqvS9iIh8FU0usJZQ#Constructor-speed-of-STL-vector-on-Pi4b

    std::vector<std::reference_wrapper<TimerEvent>> event_vector_;
    
    void Run(uint32_t thread_num);
    bool RemoveFromVector(int uid);
    void InsertIntoEventVector(TimerEvent& timer_event);  // TODO:add template
    TimerEvent& PopFrontFromEventVector();
    uint32_t GetErrorOfNanosleep();
    double GetErrorOfTimerMs(double duration_ms);
    TimerEvent& CreateTimerEvent(std::function<void()> task, double period_ms);
    void HandleExpiredEvents(BS::thread_pool& pool);
    double EvalNextInterval();
    // return ms
    inline double EvalTimeDiffFromNow(std::chrono::system_clock::time_point t);

    inline bool IsExpired(TimerEvent& event) {
      return (EvalTimeDiffFromNow(event.t) > event.period_ms);
    }

    inline bool IsNearExpired(TimerEvent& event) {
      return (EvalTimeDiffFromNow(event.t) > (event.period_ms+min_valid_interval_));
    }

    inline bool HasIdleThread(BS::thread_pool pool) {
      return (pool.get_thread_count() > pool.get_tasks_running());
    };

};

}
#endif
