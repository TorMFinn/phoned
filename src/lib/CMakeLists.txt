add_library(libphoned
  handset.cpp
  dial.cpp
  ringer.cpp
  modem.cpp
  )

target_link_libraries(libphoned
  pthread
  systemd
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
