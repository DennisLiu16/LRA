// compile: g++ -o timer timer.cc timer.h -O3 -I ~/LRA/src -std=c++17 -pthread -g

#include <util/timer/timer.h>

#include <unistd.h>
#include <stdio.h>

#include <iostream>

uint32_t t = 0;
double nb = 2;
double pa_v = 0.988;
double total_time_ms = 100000.0;
uint32_t unique_uid;

void EasyPrint() {
  std::cout << "Good" << std::endl;
}

void EasyPrint2() {
  static uint32_t ll = 0;
  ll++;
  if(ll == (uint32_t)total_time_ms/nb*pa_v)
    printf("%.2f % reached\n", pa_v*100);
}



int main() {
  lra::timer_util::Timer my_timer;
  std::cout << "Create Timer" << std::endl;

  // unique_uid = my_timer.SetLoopEvent(EasyPrint,2000.0);
  // my_timer.CancelEvent(unique_uid);
  std::cout << "Cancel completed" << std::endl;

  unique_uid = my_timer.SetLoopEvent(EasyPrint2, nb);
  
  auto start = std::chrono::high_resolution_clock::now();
  my_timer.PreciseSleepms(total_time_ms, 0);
  auto end = std::chrono::high_resolution_clock::now();
  
  printf("precise timer : %.3f ms\n", (end-start).count()/1e6);

  start = std::chrono::high_resolution_clock::now();
  EasyPrint2();
  end = std::chrono::high_resolution_clock::now();

  printf("func cost : %.3f ms\n", (end-start).count()/1e6);
}