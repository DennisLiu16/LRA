# The first target makefile of LRA project

message("CMAKE_SOURCE_DIR= ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR= ${CMAKE_BINARY_DIR}")

cmake_minimum_required(VERSION 3.22)

project(LRA LANGUAGES C CXX
            VERSION 1.0
            DESCRIPTION "A project for LRA control and show data"
        )

# global var
set(CMAKE_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/src)
## debug usage ##
set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")  # O3 optimization
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")

# add_sub_directories , remember add CMakeList.txt
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)

# ref 
# https://stackoverflow.com/questions/67385282/cmake-set-compile-options-and-compile-features-per-project


