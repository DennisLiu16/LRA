#include <util/log/logunit.h>

#include <algorithm>

namespace lra::log_util {

// global no name logger
LogUnit::LogUnit() { name_ = CreateLogUnitName(""); }

LogUnit::LogUnit(const char* cstr) {
  std::string name_with_prefix("s_");
  name_with_prefix.append(cstr);
  name_ = CreateLogUnitName(name_with_prefix);
}

LogUnit::LogUnit(const std::string_view str_v) {
  std::string name_with_prefix("s_");
  name_with_prefix.append(str_v);
  name_ = CreateLogUnitName(name_with_prefix);
}

LogUnit::LogUnit(const std::string& name) {
  std::string name_with_prefix("s_");
  name_with_prefix.append(name);
  name_ = CreateLogUnitName(name_with_prefix);
}

std::string LogUnit::getName() { return name_; }

void LogUnit::DropAllLogUnits() { logunits_.clear(); }

void LogUnit::AddLogger(const std::string& logger_name) { loggers_.insert(logger_name); }

void LogUnit::AddLogger(const std::shared_ptr<spdlog::logger> logger_ptr) { loggers_.insert(logger_ptr->name()); }

void LogUnit::AddLogger(const spdlog::logger& logger) { loggers_.insert(logger.name()); }

void LogUnit::RemoveLogger(const std::string& logger_name) { loggers_.erase(logger_name); }

void LogUnit::RemoveLogger(const std::shared_ptr<spdlog::logger> logger_ptr) { loggers_.erase(logger_ptr->name()); }

void LogUnit::RemoveLogger(const spdlog::logger& logger) { loggers_.erase(logger.name()); }

std::shared_ptr<LogUnit> LogUnit::getLogUnit(const std::string& str) {
  return (logunits_.find(str) != logunits_.end()) ? logunits_[str] : nullptr;
}

std::vector<std::string> LogUnit::getAllLogUnitKeys() {
  RefreshKeys();

  std::vector<std::string> vec;
  vec.reserve(logunits_.size());

  for (auto pair : logunits_) {
    vec.push_back(pair.first);
  }

  return vec;
}

ssize_t LogUnit::RefreshKeys() {
  const ssize_t count = std::erase_if(logunits_, [](const auto& item) {
    auto const& [key, value] = item;
    return (value.use_count() == 1);
  });

  return count;
}

bool LogUnit::Drop(const std::string& logunit_name /* = "" */) {
  std::string name{logunit_name == "" ? name_ : logunit_name};

  if (logunits_[name].use_count() == 1) {
    return logunits_.erase(name);
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
