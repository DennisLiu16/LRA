message("CMAKE_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

# Find FFTW libraries and include directories
find_library(FFTW_LIBRARY
  NAMES fftw3
  HINTS /usr/lib /usr/local/lib)
find_path(FFTW_INCLUDE_DIR
  NAMES fftw3.h
  HINTS /usr/include /usr/local/include)

if(FFTW_LIBRARY AND FFTW_INCLUDE_DIR)
  message(STATUS "Found FFTW: ${FFTW_LIBRARY}")
else()
  message(FATAL_ERROR "FFTW not found")
endif()

# Create an INTERFACE library
add_library(lra_fft_lib INTERFACE)
target_include_directories(lra_fft_lib INTERFACE 
${FFTW_INCLUDE_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/file_loader
${CMAKE_CURRENT_SOURCE_DIR}/fft_wrapper)
target_link_libraries(lra_fft_lib INTERFACE ${FFTW_LIBRARY})