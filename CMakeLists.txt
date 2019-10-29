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

find_package(SDL2 REQUIRED)

add_executable(phoned_audiod
  src/bin/audiod.cpp
  )

add_executable(phoned_handsetd
  src/bin/handsetd.cpp
  )

add_library(libphoned
  src/lib/dialtone.cpp
  src/lib/handset.cpp
  )

target_link_libraries(libphoned
  SDL2::SDL2
  systemd
  )

target_include_directories(libphoned PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  )

target_link_libraries(phoned_audiod
  libphoned
  )

target_link_libraries(phoned_handsetd
  libphoned
  )

add_sanitizers(
  phoned_audiod
  phoned_handsetd
  libphoned
  )
