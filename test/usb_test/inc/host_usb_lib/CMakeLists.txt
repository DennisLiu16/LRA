message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(logger)

add_library(host_usb_lib SHARED)

# link to library
target_link_libraries(host_usb_lib PUBLIC drv_stm_lib lra_usb_logger_lib)

target_include_directories(host_usb_lib
    PUBLIC
        # access to spdlog headers
        ${SPDLOG_MACRO_OVERLAY_INCLUDE_PATH}
        
        ${CMAKE_SOURCE_DIR}/test/usb_test/inc/
        ${CMAKE_CURRENT_SOURCE_DIR}/cdcDevice
        ${CMAKE_CURRENT_SOURCE_DIR}/command
        ${CMAKE_CURRENT_SOURCE_DIR}/userInput
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CMAKE_CURRENT_SOURCE_DIR}/parser  # rcws_parser.h is in this directory
)

# add the source file to the library
target_sources(host_usb_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/rcws_parser.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/cdcDevice/rcws.cc
)

# target_include_directories(host_usb_lib PUBLIC 
# ${CMAKE_SOURCE_DIR}/test/usb_test/inc/
# ${THIRD_PARTY_INCLUDE_PATH}
# ${SRC_INCLUDE_PATH}
# )

target_link_libraries(host_usb_lib INTERFACE spdlog serial usb-1.0 udev)