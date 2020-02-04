# cmake package instructions
# for system config files

# These files may be required to be put in /etc or /usr/local
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/udev/ DESTINATION etc/udev/rules.d)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned_handsetd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned_handsetd.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned_modemd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned_modemd.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned_diald.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned_diald.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned_audiod.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned_audiod.service)
             
configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned_ringerd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned_ringerd.service)

INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/systemd/ DESTINATION etc/systemd/system)
