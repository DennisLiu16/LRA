// compile: g++ -o timer timer.cc timer.h -O3 -I ~/LRA/src -std=c++17 -pthread -g

#include <util/timer/timer.h>

#include <unistd.h>

#include <iostream>

void EasyPrint() {
  std::cout << "Good" << std::endl;
}

void EasyPrint2() {
  static uint32_t ll = 0;
  ll++;
  if(ll % 100 ==0)
    std::cout << ll << std::endl;
}

uint32_t t = 0;

int main() {
  lra::timer_util::Timer my_timer;
  std::cout << "Create Timer" << std::endl;

  //  unique_uid = my_timer.SetEvent(EasyPrint,3000.0);
  // my_timer.CancelEvent(unique_uid);
  std::cout << "Cancel completed" << std::endl;

  uint32_t unique_uid = my_timer.SetLoopEvent(EasyPrint2, 1.0);
  sleep(10);
}