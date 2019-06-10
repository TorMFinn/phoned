cmake_minimum_required(VERSION 3.0)
project(phone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0)

add_executable(phoned
  src/phoned.cpp
  )

add_library(phoned-lib
  src/audio.cpp
  src/handset.cpp
  src/modem.cpp
  src/dial.cpp
  src/ringer.cpp
  )

target_link_libraries(phoned-lib
  gpio
  ${GST_LIBRARIES}
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
