message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(lra_realtime_lib SHARED)

target_link_libraries(lra_realtime_lib PUBLIC spdlog host_usb_lib)

target_include_directories(lra_realtime_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# add the source file to the library
target_sources(lra_realtime_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/realtime_plot.cc
)

