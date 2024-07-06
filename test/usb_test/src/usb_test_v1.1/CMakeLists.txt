message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(lra_usb_util_test_v1.1 usb_test_v1.1.cc)

# 要出現 g++ -lxxx， 在 cmake 之中要去掉 'l'
target_link_libraries(lra_usb_util_test_v1.1 PRIVATE 
host_usb_lib
lra_fft_lib)

target_include_directories(lra_usb_util_test_v1.1 PRIVATE 
host_usb_lib
lra_fft_lib
)

# set to bin dir
set_target_properties(lra_usb_util_test_v1.1
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(lra_usb_util_test_v1.1 PRIVATE RCWS_LRA_DATA_PATH=\"${RCWS_LRA_DATA_PATH}\")
target_compile_definitions(lra_usb_util_test_v1.1 PRIVATE RCWS_LRA_ROOT_PATH=\"${RCWS_LRA_ROOT_PATH}\")