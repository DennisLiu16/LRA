add_executable(temp_spdlog_test temp_spdlog_test.cc)

target_include_directories(temp_spdlog_test PRIVATE ${SRC_INCLUDE_PATH})
target_include_directories(temp_spdlog_test PRIVATE ${THIRD_PARTY_INCLUDE_PATH})

# at lease enable 1
target_include_directories(temp_spdlog_test PRIVATE ${SPDLOG_MACRO_OVERLAY_INCLUDE_PATH})
# target_link_libraries(temp_spdlog_test spdlog)

# i2c is from usr/include/i2c lib ; I2C is located in this project repo
