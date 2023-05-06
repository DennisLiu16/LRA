message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(host_usb_lib INTERFACE)

target_include_directories(host_usb_lib
    INTERFACE
        spdlog

        ${CMAKE_SOURCE_DIR}/test/usb_test/inc/
        
        ${CMAKE_CURRENT_SOURCE_DIR}/cdcDevice
        ${CMAKE_CURRENT_SOURCE_DIR}/command
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/userInput
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
)

# target_include_directories(host_usb_lib PUBLIC 
# ${CMAKE_SOURCE_DIR}/test/usb_test/inc/
# ${THIRD_PARTY_INCLUDE_PATH}
# ${SRC_INCLUDE_PATH}
# )

target_link_libraries(host_usb_lib INTERFACE spdlog serial usb-1.0 udev)



