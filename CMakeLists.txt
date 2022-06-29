# The first target makefile of LRA project
cmake_minimum_required(VERSION 3.22)

message("CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")



project(LRA LANGUAGES C CXX
            VERSION 1.0
            DESCRIPTION "A project for LRA control and data transfer"
        )

# include path
set(SRC_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/src)
set(THIRD_PARTY_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party)

# build info 
set(CMAKE_VERBOSE_MAKEFILE TRUE)

# CXX compiler general parameters
set(CMAKE_CXX_FLAGS "-Wall")

# CXX compiler DEBUG parameters
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -O0 -ftime-trace")

# CXX compiler RELEASE parameters
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")  # O3 optimization, close assert by define NDEBUG

# C compiler general parameters
set(CMAKE_C_FLAGS "-Wall")

# C compiler DEBUG parameters
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -O0")

# C compiler RELEASE parameters
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

# build type
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
else()
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# options
## log related: see src/util/log/CMakeLists.txt


# sub directories
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party)

# ref 
# https://stackoverflow.com/questions/67385282/cmake-set-compile-options-and-compile-features-per-project


