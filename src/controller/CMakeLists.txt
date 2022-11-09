message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_controller SHARED ${SRC})

target_include_directories(lra_controller PUBLIC lra_device_drv2605l lra_device_adxl355 lra_device_tca lra_log_util lra_bus_i2c)

target_link_libraries(lra_controller PUBLIC lra_device_drv2605l lra_device_adxl355 lra_device_tca lra_log_util lra_bus_i2c)