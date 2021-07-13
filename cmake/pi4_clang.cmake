set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

set(CMAKE_C_COMPILER_TARGET armv7l-unknown-linux-gnueabihf)
set(CMAKE_CXX_COMPILER_TARGET armv7l-unkonwn-linux-gnueabihf)
set(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN /home/tmf/x-tools/arm-unknown-linux-gnueabihf)
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN /home/tmf/x-tools/arm-unknown-linux-gnueabihf)

set(CMAKE_SYSROOT /home/tmf/x-tools/arm-unknown-linux-gnueabihf)
