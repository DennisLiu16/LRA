# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

# header file only
add_library(lra_terminal_util INTERFACE)

target_include_directories(lra_terminal_util INTERFACE .)