message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_device_adxl355 SHARED ${SRC})
find_library(wiringPi_LIB wiringPi)

target_include_directories(lra_device_adxl355 PUBLIC ${SRC_INCLUDE_PATH} lra_memory_registers lra_log_util)

# abort wiringPi in the future
target_link_libraries(lra_device_adxl355 PUBLIC lra_memory_registers lra_log_util ${wiringPi_LIB})