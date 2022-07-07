#include <util/log/log.h>

namespace lra_log_util {

// implement of LogUnit
// public
LogUnit::~LogUnit() {Drop();}

void LogUnit::AddLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.insert(logger_name);
}

void LogUnit::RemoveLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.erase(logger_name);
}

bool LogUnit::IsRegistered(const std::string &logunit_name) {
  if(!Drop(logunit_name)) {
    return logunits_[logunit_name].use_count() > 0;
  }
}

std::vector<std::string> LogUnit::getAllKeys() {
  std::vector<std::string> vec{};
  for(auto map: logunits_) {
    long count = map.second.use_count();
    if(count > 1) {
      vec.push_back(map.first);
    } else if(count == 1) {
      Drop(map.first);
    }
  }
  return vec;
}

// private
void LogUnit::Register(std::shared_ptr<LogUnit> ptr) { logunits_[ptr->name_] = ptr; }

// Drop logunit only be used in logunits_
bool LogUnit::Drop(const std::string &logunit_name) {
  if(logunits_[logunit_name].use_count() == 1) {
    logunits_.erase(logunit_name);
    return true;
  }
  return false;
}

bool LogUnit::Drop() {  // should use when shared_ptr.use_count() == 1, so Drop->~LogUnit()
  if(logunits_[name_].use_count() == 1) {
    logunits_.erase(name_);
    return true;
  }
  return false;
}

// static variable init
uint32_t LogUnit::idx_for_next_ = 0;
std::unordered_map<std::string, std::shared_ptr<LogUnit>> LogUnit::logunits_{};

}  // namespace lra_log_util