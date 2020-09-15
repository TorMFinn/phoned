# cmake package instructions
# for system config files

# These files may be required to be put in /etc or /usr/local
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/udev/ DESTINATION etc/udev/rules.d)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned-handsetd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned-handsetd.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned-modemd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned-modemd.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned-diald.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned-diald.service)

configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned-audiod.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned-audiod.service)
             
configure_file(${CMAKE_CURRENT_LIST_DIR}/systemd/phoned-ringerd.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/systemd/phoned-ringerd.service)

INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/systemd/ DESTINATION etc/systemd/system)
