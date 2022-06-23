file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_timer_util SHARED ${SRC})

find_package(Threads REQUIRED)

target_link_libraries(lra_timer_util PRIVATE Threads::Threads)
target_include_directories(lra_timer_util PRIVATE ${CMAKE_INCLUDE_PATH})
target_compile_features(lra_timer_util PRIVATE cxx_std_20)