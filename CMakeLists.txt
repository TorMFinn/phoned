cmake_minimum_required(VERSION 3.10)
project(phoned LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/ext/sanitizers-cmake/cmake" ${CMAKE_MOUDLE_PATH})
find_package(Sanitizers)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(STATUS "Enabling address sanitizers")
  set(SANITIZE_ADDRESS ON)
  set(SANITIZE_UNDEFINED ON)
endif()

add_subdirectory(src/lib)

add_subdirectory(src/bin)

include(etc/install.cmake)
