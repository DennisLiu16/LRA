message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(lra_timer_util_test timer_test.cc)

target_include_directories(lra_timer_util_test PRIVATE lra_timer_util)
target_link_libraries(lra_timer_util_test PRIVATE lra_timer_util)

# target_compile_features(lra_timer_util_test PRIVATE cxx_std_20)