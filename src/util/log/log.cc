#include <util/log/log.h>

namespace lra_log_util {

// implement of LogUnit
// public
LogUnit::~LogUnit() {}

void LogUnit::AddLogger(std::string logger_name) {  // move copy value to unordered set directly
  loggers_.insert(std::move(logger_name));
}

void LogUnit::RemoveLogger(std::string logger_name) {  // move copy value to unordered set directly
  loggers_.erase(std::move(logger_name));
}

bool LogUnit::IsRegistered(std::string logunit_name) {
  return logunits_.count(std::move(logunit_name));
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

}  // namespace lra_log_util