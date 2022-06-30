message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# add target
add_library(lra_log_util SHARED ${SRC})

# function include
# ref: https://cmake.org/cmake/help/latest/module/CMakeDependentOption.html#module:CMakeDependentOption
# include(CMakeDependentOption)

# options
# option won't effect Cxx macro directly, use target_compile_definitions to generate macro to src file defined in CMake
option(USE_LOG_SYSTEM
  "Determine use log system or disable it. If you decide to enable it, select SPDLOG_LEVEL also."
  ON
)

# option(USE_SPDLOG
#   "On: use spdlog as your log system; Off: use std::cout as your log system."
#   ON
# )

# flag check
if(USE_LOG_SYSTEM)
  message(STATUS "Use spdlog as log system")
  target_compile_definitions(lra_log_util PUBLIC USE_LOG_SYSTEM)

  # Select SPDLOG_LEVEL

  # aborted
  # if(USE_SPDLOG)
  #   message(STATUS "Use spdlog as log system")
  #   target_link_libraries(lra_log_util spdlog)
  #   target_compile_definitions(lra_log_util PRIVATE USE_SPDLOG)
  # else()
  #   message(STATUS "Use std::cout as log system")
  #   target_compile_definitions(lra_log_util PRIVATE USE_STDLOG)
  # endif()

else()
  message(STATUS "Disable log system")
  # define nothing
endif()

target_link_libraries(lra_log_util spdlog)
target_include_directories(lra_log_util PRIVATE ${SRC_INCLUDE_PATH})
# target_compile_features(lra_log_util PRIVATE cxx_std_20)

# 可以用 cmake -LH 查看 options 也可以透過 tab 看 -D flag
# TODO:https://runebook.dev/zh-CN/docs/cmake/command/target_precompile_headers precompile