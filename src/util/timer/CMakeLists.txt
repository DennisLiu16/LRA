message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_timer_util SHARED ${SRC})

find_package(Threads REQUIRED)

target_link_libraries(lra_timer_util PRIVATE Threads::Threads lra_log_util)

target_include_directories(lra_timer_util PRIVATE ${SRC_INCLUDE_PATH})
target_include_directories(lra_timer_util PUBLIC ${THIRD_PARTY_INCLUDE_PATH})   # file include lra_timer_util need to know this path

# target_compile_features(lra_timer_util PRIVATE cxx_std_20)