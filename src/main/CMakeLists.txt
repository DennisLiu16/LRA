message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(lra_main main.h main.cc)

target_include_directories(lra_main PRIVATE lra_log_util lra_timer_util lra_controller)
target_link_libraries(lra_main PRIVATE lra_log_util lra_timer_util lra_controller)

# set to bin dir
set_target_properties(lra_main
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)