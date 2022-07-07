# The first target makefile of LRA project
cmake_minimum_required(VERSION 3.22)

# option define
## USE_LOG_SYSTEM
option(USE_LOG_SYSTEM
  "Determine use log system or disable it. If you decide to enable it, default to use spdlog."
  ON
)

## SPDLOG_LEVEL
set(SPDLOG_LEVEL_OPTIONS Trace Debug Info Warn Error Critical)
set(SPDLOG_LEVEL "Trace" CACHE STRING
"SPDLOG_LEVEL chosen by the user at CMake configure time")
set_property(CACHE SPDLOG_LEVEL PROPERTY STRINGS ${SPDLOG_LEVEL_OPTIONS})

##############################################@@ USER DEFINE @@########################################################
# user defined options
# option won't effect Cxx macro directly, use target_compile_definitions to generate macro to src file defined in CMake

## option - USE_LOG_SYSTEM , **default ON**. You can use -DUSE_LOG_SYSTEM to toggle or directly set here.
# >>> your option... <<<

## list SPDLOG_LEVEL, valid if USE_LOG_SYSTEM is ON, **default Trace**.
# should be one of "Trace", "Debug", "Info", "Warn ", "Error", "Critical"
# Only valid for logunit
set(SPDLOG_LEVEL "Info")

########################################################################################################################

# color settings
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset "${Esc}[m")
  set(ColorBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/gcc")


project(LRA LANGUAGES C CXX
            VERSION 1.0
            DESCRIPTION "A project for LRA control and data transfer"
        )

message("CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

# include path
set(SRC_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/src)
set(THIRD_PARTY_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party)
# This include path is for test code that not want to link lib spdlog or lra_log_util
set(SPDLOG_MACRO_OVERLAY_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party/spdlog/include) 

# build info 
set(CMAKE_VERBOSE_MAKEFILE TRUE)

# standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# CXX compiler general parameters
set(CMAKE_CXX_FLAGS "-Wall")

# CXX compiler DEBUG parameters
# if without flag -fPIC, spdlog will be compiled without -fPIC while src/* will be compiled with -fPIC -> link failed
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -O0 -fPIC")  

# CXX compiler RELEASE parameters
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -fPIC")  # O3 optimization, close assert by define NDEBUG

# C compiler general parameters
set(CMAKE_C_FLAGS "-Wall")

# C compiler DEBUG parameters
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -O0")

# C compiler RELEASE parameters
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

# set default build type
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
endif()

message(STATUS "${BoldRed}${CMAKE_PROJECT_NAME} Build type: ${CMAKE_BUILD_TYPE} ${ColorReset}")

# sub directories
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party)

# ref 
# https://stackoverflow.com/questions/67385282/cmake-set-compile-options-and-compile-features-per-project


