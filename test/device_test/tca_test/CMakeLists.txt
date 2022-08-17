# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(lra_device_test_tca tca_test.cc)

target_include_directories(
  lra_device_test_tca 
  PRIVATE 
  lra_device_tca
)

target_link_libraries(lra_device_test_tca PRIVATE lra_device_tca)


add_executable(lra_i2c_directly_call_test i2c_directly_call_test.cc)

target_include_directories(
  lra_i2c_directly_call_test
  PRIVATE 
  i2c
)

target_link_libraries(lra_i2c_directly_call_test PRIVATE i2c)
