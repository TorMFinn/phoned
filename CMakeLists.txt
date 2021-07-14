cmake_minimum_required(VERSION 3.17)
project(phoned LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/ext/sanitizers-cmake/cmake" ${CMAKE_MOUDLE_PATH})
find_package(Sanitizers)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(STATUS "Enabling address sanitizers")
  set(SANITIZE_ADDRESS ON)
  set(SANITIZE_UNDEFINED ON)
endif()

# Use CCACHE if it is installed
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
  message(STATUS "Found ccache")
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Add SDBUS-CPP dependency from kistler-group
#add_subdirectory(ext/sdbus-cpp)

add_subdirectory(src/lib)

add_subdirectory(src/bin)

include(etc/install.cmake)
