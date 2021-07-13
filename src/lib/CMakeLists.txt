include(FindPkgConfig)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

add_library(libphoned
  handset.cpp
  dial.cpp
  ringer.cpp
  modem.cpp
  dialtone.cpp
  #audio.cpp
  audio_gst.cpp
  )

target_link_libraries(libphoned
  pthread
  systemd
  pulse-simple
  SDL2
  ${GSTREAMER_LIBRARIES}
  )

target_include_directories(libphoned PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  )

target_include_directories(libphoned PRIVATE
  ${GSTREAMER_INCLUDE_DIRS}
  )

set_target_properties(libphoned PROPERTIES OUTPUT_NAME phoned)

# Cmake Install packaging
INSTALL(TARGETS libphoned
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  )
