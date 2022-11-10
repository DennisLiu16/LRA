message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(lra_websocket SHARED ${SRC})

set(ASIO_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party/asio/asio/include)
set(WEBSOCKETPP_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party/websocketpp/)
set(JSONCPP_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/third_party/jsoncpp/include)

target_include_directories(lra_websocket PUBLIC ${SRC_INCLUDE_PATH} ${WEBSOCKETPP_INCLUDE_PATH} ${JSONCPP_INCLUDE_PATH} ${ASIO_INCLUDE_PATH} lra_log_util)
target_link_libraries(lra_websocket PUBLIC lra_log_util)