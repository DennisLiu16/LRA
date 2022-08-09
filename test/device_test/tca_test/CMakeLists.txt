# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(lra_device_test_tca ${SRC})

target_include_directories(
  lra_device_test_tca 
  PRIVATE 
  lra_device_tca
)

target_link_libraries(lra_device_test_tca PRIVATE lra_device_tca)
# target_compile_options(lra_device_test_tca PRIVATE -E)