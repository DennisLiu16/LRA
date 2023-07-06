message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(lra_usb_logger_lib SHARED)

target_link_libraries(lra_usb_logger_lib PUBLIC spdlog)

target_include_directories(lra_usb_logger_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# add the source file to the library
target_sources(lra_usb_logger_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cc
)