SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(phoned
  phoned.cpp
  )

target_link_libraries(phoned
  libphoned
  )

add_executable(test-audio
  test_audio.cpp
  )

target_link_libraries(test-audio
  libphoned
  )

INSTALL(TARGETS
  phoned
  RUNTIME DESTINATION bin
  )
