# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_bus_i2c SHARED ${SRC})
                    
# lra_log_util include path has ${SRC_INCLUDE_PATH}, but we reclaim here to avoid ambiguous
target_include_directories(lra_bus_i2c PUBLIC ${SRC_INCLUDE_PATH})
# smbus related function depends on li2c (from i2c-tools) from version 4.0
target_link_libraries(lra_bus_i2c PUBLIC lra_terminal_util i2c) 