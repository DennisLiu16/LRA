# Add in branch i2c_unittest
message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_busadapter_i2cadapter SHARED ${SRC})

# lra_log_util include path has ${SRC_INCLUDE_PATH} and ${THIRD_PARTY_INCLUDE_PATH}, but we reclaim here to avoid ambiguous ?
target_include_directories(lra_busadapter_i2cadapter PUBLIC lra_log_util)

target_link_libraries(lra_busadapter_i2cadapter 
                      PUBLIC
                      lra_log_util
                      lra_concepts_util
                      lra_errors_util
                      lra_memory_registers
                      lra_bus_i2c
                      )
                    

