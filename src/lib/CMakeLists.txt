add_library(libphoned
  dialtone.cpp
  handset.cpp
  dial.cpp
  ringer.cpp
  modem.cpp
  modem_audio.cpp
  )

target_link_libraries(libphoned
  pthread
  systemd
  pulse-simple
  pulse
  )

target_include_directories(libphoned PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  )

set_target_properties(libphoned PROPERTIES OUTPUT_NAME phoned)

add_sanitizers(libphoned)

# Cmake Install packaging

INSTALL(TARGETS libphoned
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  )
