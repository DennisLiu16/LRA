#include <util/log/log.h>

namespace lra_log_util {

// implement of LogUnit
// public
LogUnit::~LogUnit() {}

void LogUnit::AddLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.insert(logger_name);
}

void LogUnit::RemoveLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.erase(logger_name);
}

bool LogUnit::IsRegistered(const std::string &logunit_name) {
  return logunits_.count(logunit_name);
}

std::vector<std::string> LogUnit::getAllKeys() {
  std::vector<std::string> vec{};
  for(auto map: logunits_) {
    vec.push_back(map.first);
  }
  return vec;
}

// private
void LogUnit::Register(std::shared_ptr<LogUnit> ptr) { logunits_[ptr->name_] = ptr; }

// 要搞清楚順序
// 放在 ~LogUnit 會錯是因為只有當 use_count == 0 也就是物件不存在時才會 call destructor -> invalid
// operation ? used in class' destruor
void LogUnit::Drop() {  // should use when shared_ptr.use_count() == 1, so Drop->~LogUnit()
  std::cout << "left " << logunits_[name_].use_count() << std::endl;
  std::cout << "remove " + name_ << std::endl;
  logunits_.erase(name_);
}

// static variable init
uint32_t LogUnit::idx_for_next_ = 0;
std::unordered_map<std::string, std::shared_ptr<LogUnit>> LogUnit::logunits_{};

}  // namespace lra_log_util