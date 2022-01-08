add_library(libphoned
  handset.cpp
  dial.cpp
  contacts.cpp
  dialtone_sdl.cpp
  ringer.cpp
  modem.cpp
  )

target_link_libraries(libphoned PRIVATE
  pthread
  SQLiteCpp
  ${SDL2_LIBRARIES}
  )

  target_include_directories(libphoned PRIVATE
    ${SDL2_INCLUDE_DIRS}
  )

target_include_directories(libphoned PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  )

set_target_properties(libphoned PROPERTIES OUTPUT_NAME phoned)

add_sanitizers(libphoned)

if (PHONED_TESTING)
  add_subdirectory(tests)
endif()

# Cmake Install packaging

INSTALL(TARGETS libphoned
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  )
