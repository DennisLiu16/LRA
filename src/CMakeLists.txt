add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/util)

# Add in branch i2c_unittest
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bus)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bus_adapter)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/memory)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/device)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/controller)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/websocket)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/main)
