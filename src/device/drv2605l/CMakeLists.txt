message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_device_drv2605l SHARED ${SRC})

target_include_directories(lra_device_drv2605l PUBLIC lra_busadapter_i2cadapter lra_log_util)

target_link_libraries(lra_device_drv2605l PUBLIC lra_busadapter_i2cadapter lra_log_util)