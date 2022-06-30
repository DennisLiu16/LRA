// define macro here to test
#include <set>
#include <spdlog/spdlog.h>

void GenerateLoggerInFunction() {
  // create logger
  // add handle here
}

void PrintAllLogger() {
  static std::set<std::string> myset;
  spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l){myset.insert(l->name());});

  // print all
  
}

int main () {

}

// conclusion
// 1. 在 func 可以建立 logger 並可以用 apply_all 查詢