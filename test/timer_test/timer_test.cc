// compile: g++ -o timer timer.cc timer.h -O3 -I ~/LRA/src -std=c++17 -pthread -g

#include <util/timer/timer.h>

#include <unistd.h>
#include <stdio.h>

#include <iostream>

uint32_t t = 0;
double nb = 10;
double pa_v = 0.99;
double total_time_ms = 1000.0;
uint32_t unique_uid;

void EasyPrint() {
  
}

void EasyPrint2() {
  static uint32_t ll = 0;
  ll++;
  printf("%d\n", ll);
  // if(ll == (uint32_t)total_time_ms/nb*pa_v)
  //   printf("%.2f % reached\n", pa_v*100);
}

void EasyPrint3() {
  usleep(9500);
  static uint32_t ll = 0;
  ll++;
  if(ll == (uint32_t)total_time_ms/nb*pa_v)
    printf("%.2f % reached\n", pa_v*100);
}

int main() {
  lra::timer_util::Timer my_timer;
  std::cout << "Create Timer" << std::endl;

  unique_uid = my_timer.SetLoopEvent(EasyPrint,100.0);
  my_timer.SetLoopEvent(EasyPrint2,nb);
  // my_timer.CancelEvent(unique_uid);
  std::cout << "Cancel completed" << std::endl;

  unique_uid = my_timer.SetLoopEvent(EasyPrint3, nb);
  double total = 0.0;
  int count = 0;
  

  auto start = std::chrono::high_resolution_clock::now();
  // my_timer.PreciseSleepms(total_time_ms, 0);
    my_timer.PreciseSleepms(100000, 0);
  auto end = std::chrono::high_resolution_clock::now();
}