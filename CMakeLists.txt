cmake_minimum_required(VERSION 3.0)
project(phone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (FreeBSD)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
endif()

find_package(PkgConfig)
find_package(SDL2 REQUIRED)

add_executable(phoned
  src/phoned.cpp
  )

add_library(phoned-lib
  src/dialtone_sdl.cpp
  #src/handset.cpp
  #src/modem.cpp
  #src/modem_audio.cpp
  #src/dial.cpp
  #src/ringer.cpp
  )

target_link_libraries(phoned-lib
  #gpio
  SDL2::SDL2
  )

target_include_directories(phoned-lib PRIVATE SYSTEM
  ${GST_INCLUDE_DIRS}
  )

target_include_directories(phoned-lib PUBLIC
  src
  )

target_link_libraries(phoned
  phoned-lib
  pthread
  )
