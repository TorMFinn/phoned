SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(phoned-diald
  diald.cpp
)

add_executable(phoned-handsetd
  handsetd.cpp
  )

add_executable(phoned-ringerd
  ringerd.cpp
)

add_executable(phoned-modemd
  modemd.cpp
)

target_link_libraries(phoned-handsetd PRIVATE
  libphoned
  )

target_link_libraries(phoned-diald PRIVATE
  libphoned
)

target_link_libraries(phoned-ringerd PRIVATE
  libphoned
)

target_link_libraries(phoned-modemd PRIVATE
  libphoned
)

add_sanitizers(
  phoned-ringerd
  phoned-modemd
  phoned-handsetd
  phoned-diald
  )

INSTALL(TARGETS
  phoned-handsetd
  phoned-diald
  phoned-modemd
  phoned-ringerd
  RUNTIME DESTINATION bin
  )
