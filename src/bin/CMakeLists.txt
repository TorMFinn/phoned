add_executable(phoned_audiod
  audiod.cpp
  )

add_executable(phoned_diald
  diald.cpp
)

add_executable(phoned_handsetd
  handsetd.cpp
  )

add_executable(phoned_ringerd
  ringerd.cpp
)

add_executable(phoned_modemd
  modemd.cpp
)

target_link_libraries(phoned_audiod PRIVATE
  libphoned
  )

target_link_libraries(phoned_handsetd PRIVATE
  libphoned
  )

target_link_libraries(phoned_diald PRIVATE
  libphoned
)

target_link_libraries(phoned_ringerd PRIVATE
  libphoned
)

target_link_libraries(phoned_modemd PRIVATE
  libphoned
)

add_sanitizers(
  phoned_ringerd
  phoned_modemd
  phoned_audiod
  phoned_handsetd
  phoned_diald
  )

INSTALL(TARGETS
  phoned_handsetd
  phoned_diald
  phoned_modemd
  phoned_audiod
  phoned_ringerd
  RUNTIME DESTINATION bin
  )
