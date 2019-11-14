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

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0)

add_executable(phoned_audiod
  src/bin/audiod.cpp
  )

add_executable(phoned_diald
  src/bin/diald.cpp
)

add_executable(phoned_handsetd
  src/bin/handsetd.cpp
  )

add_executable(phoned_ringerd
  src/bin/ringerd.cpp
)

add_executable(phoned_modemd
  src/bin/modemd.cpp
)

add_library(libphoned
  src/lib/dialtone.cpp
  src/lib/handset.cpp
  src/lib/dial.cpp
  src/lib/ringer.cpp
  src/lib/modem.cpp
  #src/lib/modem_audio.cpp
  src/lib/modem_audio_sdl.cpp
  #src/lib/modem_audio_asio.cpp
  )

target_link_libraries(libphoned
  ${GST_LIBRARIES}
  pthread
  systemd
  SDL2
  )

target_include_directories(libphoned PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  )

target_include_directories(libphoned PRIVATE
  ${GST_INCLUDE_DIRS}
  )

target_link_libraries(phoned_audiod
  libphoned
  )

target_link_libraries(phoned_handsetd
  libphoned
  )

target_link_libraries(phoned_diald
  libphoned
)

target_link_libraries(phoned_ringerd
  libphoned
)

target_link_libraries(phoned_modemd
  libphoned
)

add_sanitizers(
  phoned_audiod
  phoned_handsetd
  phoned_diald
  libphoned
  )
