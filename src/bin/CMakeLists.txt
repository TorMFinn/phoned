SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(phoned
  phoned.cpp
  )

target_link_libraries(phoned PRIVATE
  libphoned
  )

add_executable(phonectl
  phonectl.cpp
  )

target_link_libraries(phonectl
  libphoned
)

add_sanitizers(
  phoned
)

INSTALL(TARGETS
  phoned
  RUNTIME DESTINATION bin
  )
