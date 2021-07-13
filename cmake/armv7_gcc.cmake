set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_SYSROOT /home/tmf/x-tools/arm-unknown-linux-gnueabihf)
set(tools /home/tmf/x-tools/arm-unknown-linux-gnueabihf)
set(CMAKE_C_COMPILER ${tools}/bin/cc)
set(CMAKE_CXX_COMPILER ${tools}/bin/c++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
