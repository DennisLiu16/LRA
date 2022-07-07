message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# add target
add_library(lra_log_util SHARED ${SRC})

## maybe you can modify this part
# option(USE_SPDLOG
#   "On: use spdlog as your log system; Off: use std::cout as your log system."
#   ON
# )

# flag check
if(USE_LOG_SYSTEM)
  message(STATUS "${BoldGreen}Enable spdlog and use it as log system ${ColorReset}")
  target_compile_definitions(lra_log_util PUBLIC USE_LOG_SYSTEM)

  set_property(CACHE SPDLOG_LEVEL PROPERTY STRINGS trace debug info warn error critical)

  if(NOT SPDLOG_LEVEL IN_LIST SPDLOG_LEVEL_OPTIONS)
    message(FATAL_ERROR 
    "\ 
    SPDLOG_LEVEL: '${SPDLOG_LEVEL}' is not in SPDLOG_LEVEL_OPTIONS. \ 
    Must be one of ${SPDLOG_LEVEL_OPTIONS}. \
    ")
  else()
    message(STATUS "${BoldMagenta}SPDLOG_LEVEL='${SPDLOG_LEVEL}' ${ColorReset}")
  endif()

  if(SPDLOG_LEVEL STREQUAL "Trace")
    target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  elseif(SPDLOG_LEVEL STREQUAL "Debug")
    target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
  elseif(SPDLOG_LEVEL STREQUAL "Info")
    target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
  elseif(SPDLOG_LEVEL STREQUAL "Warn")
    target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN)
  elseif(SPDLOG_LEVEL STREQUAL "Error")
    target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERR)
  else(SPDLOG_LEVEL STREQUAL "Critical")
  target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_CRITICAL)
  endif()

else()
  message(STATUS "${BoldRed}Disable log system ${ColorReset}")
  target_compile_definitions(lra_log_util PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)
endif()

target_link_libraries(lra_log_util PUBLIC spdlog)
target_include_directories(lra_log_util PUBLIC ${SRC_INCLUDE_PATH} ${THIRD_PARTY_INCLUDE_PATH})
target_compile_options(lra_log_util PUBLIC "-fpermissive") # enable nullptr to bool inplicit change

# 可以用 cmake -LH 查看 options 也可以透過 tab 看 -D flag
# TODO:https://runebook.dev/zh-CN/docs/cmake/command/target_precompile_headers precompile