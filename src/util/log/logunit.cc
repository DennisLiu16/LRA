#include <util/log/logunit.h>

namespace lra::log_util {

LogUnit::LogUnit() { name_ = CreateLogUnitName(""); }

LogUnit::LogUnit(const char* cstr) {
  LogUnit(std::string(cstr));
}

LogUnit::LogUnit(const std::string_view str_v) {
  LogUnit(std::string(str_v));
}

LogUnit::LogUnit(const std::string& name) {
  std::string name_with_prefix("s_");
  name_with_prefix.append(name);
  name_ = CreateLogUnitName(name_with_prefix);
}

LogUnit::~LogUnit() { Drop(); }

std::string LogUnit::getName() { return name_; }

void LogUnit::AddLogger(const std::string& logger_name) { loggers_.insert(logger_name); }

void LogUnit::RemoveLogger(const std::string& logger_name) { loggers_.erase(logger_name); }

std::shared_ptr<LogUnit> LogUnit::getLogUnit(const std::string& str) {
  return (logunits_.find(str) != logunits_.end()) ? logunits_[str] : nullptr;
}

bool LogUnit::Drop(const std::string& logunit_name /* = "" */) {
  std::string name{logunit_name == "" ? name_ : logunit_name};

  if (logunits_[name].use_count() == 1) {
    logunits_.erase(name);
    return true;
  }
  return false;
}

// get unique name: prefix_idx until unique name not exist in logunits_
std::string LogUnit::CreateLogUnitName(const std::string& prefix) {
  uint32_t idx{0};
  std::string full_name{prefix};
  full_name.append("_");
  std::string target{""};

  do {
    target.clear();
    target = full_name + std::to_string(idx++);
  } while (logunits_.find(target) != logunits_.end());

  return target;
}

void LogUnit::Register(std::shared_ptr<LogUnit> ptr) { logunits_[ptr->name_] = ptr; }

// static variables init
std::unordered_map<std::string, std::shared_ptr<LogUnit>> LogUnit::logunits_{};

}  // namespace lra::log_util
