message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(lra_usb_util_test_v1.0 usb_test_v1.0.cc)

# 要出現 g++ -lxxx， 在 cmake 之中要去掉 'l'
target_link_libraries(lra_usb_util_test_v1.0 PRIVATE serial usb-1.0 udev lra_log_util)
target_include_directories(lra_usb_util_test_v1.0 PRIVATE serial usb-1.0 udev lra_log_util)

# set to bin dir
set_target_properties(lra_usb_util_test_v1.0
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

