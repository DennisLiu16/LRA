#include <util/timer/timer.h>

#include <sys/time.h>

#include <array>
#include <algorithm>
#include <cmath>


#include <util/log/log.h>

namespace lra::timer_util {

namespace chrono = std::chrono;

explicit
Timer::Timer
(uint32_t thread_num = std::thread::hardware_concurrency()/2, DelayOpt opt = DelayOpt::kDefaultDelay) {
  if(opt == DelayOpt::kDefaultDelay) { // set to 70 us
    nanosleep_delay_us_ = 70;
  }
  
  if (nanosleep_delay_us_ == 0) { // get nanosleep average delay ~ thread::sleep_for
    nanosleep_delay_us_ = GetErrorOfNanosleep();
  }

  Run(thread_num);

}

Timer::~Timer() {

}

int Timer::SetEvent(std::function<void()> task, double duration_ms) {

  // update push_flag_

}

int Timer::SetLoopEvent(std::function<void()> task, double period_ms) {
  // TODO: warning: too short period might crush
}

bool Timer::CancelEvent(int uid) {
  if(uid >= uid_for_next_event_) { //TODO: warning: uid not exists

    return;
  }
  
  RemoveFromVector(uid);
  
}

// delete the timer instance
void Timer::Delete() {
  // stop the Timer
  run_flag_ = false;

  Timer::~Timer();
}

bool Timer::PreciseSleepms(double ms, bool enable_interrupted_by_new_event = 0) {
  // critical value for raspberry 4b
  bool larger_than_7ms = (ms >= 7.0);
  static double mean, estimate = (larger_than_7ms)?5:0.5; mean = estimate;
  static double m2 = 0;
  static uint64_t count = 1;
  auto sleep_period = (larger_than_7ms)? chrono::milliseconds(1) : chrono::microseconds(nanosleep_delay_us_);
  double X = (larger_than_7ms) ? 1 : 2;

  while(ms > estimate) { // thread sleep
    auto start = chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(sleep_period);
    auto end = chrono::high_resolution_clock::now();

    double observed = (end - start).count() / 1e6;  // to ms
    ms -= observed;

    count++; // put here for below calculation

    double delta = observed - mean;
    mean += delta / count;
    m2   += delta * (observed - mean);
    double stddev = sqrt(m2 / (count - 1));
    estimate = mean + X*stddev;
    if(!(enable_interrupted_by_new_event && push_flag_))
      return true;
  }

  // spin lock
  auto start = chrono::high_resolution_clock::now();
  while ((chrono::high_resolution_clock::now() - start).count() / 1e6 < ms) {
    if(!(enable_interrupted_by_new_event && push_flag_))
      return true;
  };
  return false;
}

// private

// create a bcakground thread to monitor event_vector_
void Timer::Run(uint32_t thread_num) {
  
  // make thread pool
  BS::thread_pool pool(thread_num);

  t_now_ = chrono::high_resolution_clock::now();
  

  // main
  while(run_flag_) {
    static double sleep_time_ms = 0.0;

    if(event_vector_.empty()) { // if event_vector is empty, sleep 1 sec
      sleep_time_ms = 1000.0;
    } else { // expired check(not empty), do task if poll ok, sort the events, calculate next closest interval
      push_flag_ = false;
      HandleExpiredEvents(pool);
      double next_interval_ms = EvalNextInterval();
      
      sleep_time_ms = next_interval_ms;
    }

    PreciseSleepms(sleep_time_ms, true); // can be interrupted
  }

  // stop all running task in thread pool

  // TODO: delete all TimerEvent in event_vector_
}

bool Timer::RemoveFromVector(int uid) {
  // ignoring TimerEvent of uid is running in thread_pool_
  // because the task will end up completation
  
  auto it = 
    std::find_if(event_vector_.begin(), event_vector_.end()
      ,[uid](TimerEvent& x){return x.uid == uid;});

  if(it != event_vector_.end()) { //bingo, remove that TimerEvent
    
    // note that if you need to delete mutiple task(maybe not unique)
    // use remove_if not find_if instead

    // store the TimerEvent for deleting
    // TODO:must debug here, I think it's wrong
    TimerEvent& event_will_be_removed = *it;
    //erase from event_vector_
    event_vector_.erase(it);
    // delete the pointer point to TimerEvent
    delete &event_will_be_removed;
    //TODO: info: remove successfully
  }

  //TODO: warning: Target TimerEvent has been deleted
  
}

void Timer::InsertIntoEventVector(TimerEvent& timer_event) {

}

TimerEvent& Timer::PopFrontFromEventVector() {

}

// use gettimeofday() to evaluate delay(us) in nanosleep
// pass unit test
uint32_t Timer::GetErrorOfNanosleep() {
  constexpr std::array<uint32_t, 20> delay = 
      {500000, 100000, 50000, 10000, 1000, 900, 500, 100, 10, 1};

  timespec req;
  timeval tval_begin, tval_end;
  uint32_t t_diff = 0;

  for(auto& it : delay) {

    req.tv_sec = it/1000000;
    // ref : https://www.zhihu.com/question/22747596
    // general optimization of mod: a%b ~ (a - a/b * b) if b is a constant
    req.tv_nsec = (it - it/1000000 * 1000000) * 1000;

    gettimeofday(&tval_begin, nullptr);
    int ret = nanosleep(&req, nullptr);
    if(-1 == ret) {
      //TODO:log nanosleep doesn't support
    }
    gettimeofday(&tval_end, nullptr);
    t_diff += (tval_end.tv_sec - tval_begin.tv_sec) * 1000000 
              + tval_end.tv_usec - tval_begin.tv_usec - it;
  }

  t_diff /= delay.size();
  //TODO:log t_diff

  return t_diff;
}


double Timer::GetErrorOfTimerMs(double duration_ms) {
  //TODO:shared_ptr?;
}

// add a TimerEvent
TimerEvent& Timer::CreateTimerEvent(std::function<void()> task, double period_ms) {
  TimerEvent& ref = *(new TimerEvent);
  ref.period_ms = period_ms;
  ref.task = task;

  // assign current time to t as birth time
  ref.t = chrono::high_resolution_clock::now();
  return ref;
}

void Timer::HandleExpiredEvents(BS::thread_pool& pool) { // events should be well sorted in vector

  if(event_vector_.empty())
    return;

  static uint64_t overloading_count = 0;

  // construct a tmp vector to store popped loop events
  std::vector<std::reference_wrapper<TimerEvent>> popped_loop_events_vector;
  int remaining_thread_num = pool.get_thread_count();
  popped_loop_events_vector.reserve(( remaining_thread_num > 8) ? remaining_thread_num : 8 );  // set vecor's capacity at least 8
  
  for(auto& event; IsExpired(event) || IsNearExpired(event) ; ) { // deal with expired and near expired (< 10 us) events
    // push back task to thread poll
    // do task, insert into sorted vector, erase

    if(!HasIdleThread(pool)) { // overloading record
      overloading_count++;
      if((overloading_count - overloading_count/10 * 10)) { // send debug log every ten times
        // TODO: debug: Timer's thread pool overloading: _
      }
    }

    TimerEvent& popped_event = PopFrontFromEventVector();

    if(event.is_loop_event) {
      // update t
      popped_loop_events_vector.push_back(popped_event); 
    }

    // notice bug in near_expired period < 10us
      

  }

  
  InsertIntoEventVector

}

double Timer::EvalNextInterval() {

}

double Timer::EvalTimeDiffFromNow(chrono::system_clock::time_point t) {
  t_now_ = chrono::high_resolution_clock::now();
  return (t_now_ - t).count() / 1e6;
}

};

