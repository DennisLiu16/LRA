# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_device_tca SHARED ${SRC})

target_include_directories(lra_device_tca PUBLIC lra_busadapter_i2cadapter)

target_link_libraries(lra_device_tca PUBLIC lra_busadapter_i2cadapter)