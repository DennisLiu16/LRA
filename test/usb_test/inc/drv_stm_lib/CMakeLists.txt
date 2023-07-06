message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(drv_stm_lib SHARED)

target_include_directories(drv_stm_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# add the source file to the library
target_sources(drv_stm_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/lra_usb_defines.cc
)




