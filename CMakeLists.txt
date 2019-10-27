cmake_minimum_required(VERSION 3.10)
project(phoned LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_executable(phoned_audiod
  src/bin/audiod.cpp
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
