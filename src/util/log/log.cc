#include <util/log/log.h>

namespace lra::log_util {

// implement of LogUnit
// public
LogUnit::LogUnit(const char* name) {
  std::string n(name);
  name_ = n.append(1, '_').append(std::to_string(idx_for_next_++));
}

LogUnit::~LogUnit() { Drop(); }

void LogUnit::AddLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.insert(logger_name); //TODO
}

void LogUnit::RemoveLogger(const std::string &logger_name) {  // move copy value to unordered set directly
  loggers_.erase(logger_name); //TODO
}

void LogUnit::AddDefaultLogger(const std::string &logger_name) {
  default_loggers_.insert(logger_name);
}

void LogUnit::RemoveDefaultLogger(const std::string &logger_name) {
  default_loggers_.erase(logger_name);
}

std::unordered_set<std::string> LogUnit::getAllDefaultLoggers() {
  return default_loggers_;
} 

bool LogUnit::IsRegisteredLogUnit(const std::string &logunit_name) { return (!Drop(logunit_name)) ? (logunits_[logunit_name].use_count() > 0) : false; }

std::vector<std::string> LogUnit::getAllLogunitKeys() {
  std::vector<std::string> vec{};
  for (auto map : logunits_) {
    long count = map.second.use_count();
    if (count > 1) {
      vec.push_back(map.first);
    } else if (count == 1) {
      Drop(map.first);
    }
  }
  return vec;
}

// private
void LogUnit::Register(std::shared_ptr<LogUnit> ptr) { logunits_[ptr->name_] = ptr; }

void LogUnit::ApplyDefaultLogger(std::shared_ptr<LogUnit> ptr) {
  for (auto logger : default_loggers_) {
    ptr->AddLogger(logger);
  }
}

// Drop logunit only be used in logunits_
bool LogUnit::Drop(const std::string &logunit_name) {
  if (logunits_[logunit_name].use_count() == 1) {
    logunits_.erase(logunit_name);
    return true;
  }
  return false;
}

bool LogUnit::Drop() {  // should use when shared_ptr.use_count() == 1, so Drop->~LogUnit()
  if (logunits_[name_].use_count() == 1) {
    logunits_.erase(name_);
    return true;
  }
  return false;
}

// static variable init
uint32_t LogUnit::idx_for_next_ = 0;
std::unordered_map<std::string, std::shared_ptr<LogUnit>> LogUnit::logunits_{};
std::unordered_set<std::string> LogUnit::default_loggers_{};
}  // namespace lra::log_util