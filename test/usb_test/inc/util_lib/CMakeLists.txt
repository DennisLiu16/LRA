message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(rcws_util_lib SHARED)

target_include_directories(rcws_util_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# add the source file to the library
target_sources(rcws_util_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/block_checker.cc
)




