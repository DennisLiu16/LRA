/**
 * dependencies: libfmt-dev
 */

#include <fmt/format.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

std::mutex input_mutex;
std::condition_variable input_cv;
std::atomic_bool input_available{false};
std::string input_buffer;

void user_input_parser(const std::string& input) { fmt::print("{}\n", input); }

void input_thread() {
  std::string line;
  while (std::getline(std::cin, line)) {
    {
      std::unique_lock<std::mutex> lock(input_mutex);
      input_buffer = std::move(line);

      // automic_bool set
      input_available.store(true, std::memory_order_release);
    }
    input_cv.notify_one();
  }
}

int main() {
  std::thread inputThread(input_thread);

  while (true) {
    std::unique_lock<std::mutex> lock(input_mutex);
    input_cv.wait(lock, [] { return input_available.load(std::memory_order_acquire); });

    std::string current_input = std::move(input_buffer);

    // automic_bool unset
    input_available.store(false, std::memory_order_release);

    if (current_input == "exit") {
      break;
    }

    user_input_parser(current_input);
  }

  inputThread.join();
  return 0;
}
