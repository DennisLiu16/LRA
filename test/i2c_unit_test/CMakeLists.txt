# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(lra_i2c_unit_test i2c_unit_test.cc)
target_include_directories(lra_i2c_unit_test PUBLIC lra_busadapter_i2cadapter lra_bus_i2c)
target_link_libraries(lra_i2c_unit_test PUBLIC lra_busadapter_i2cadapter lra_bus_i2c)