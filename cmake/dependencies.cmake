# List of dependencies required for the project

include(FindPkgConfig)

if (PHONED_SYSTEM_SQLITECPP)
    find_package(SQLiteCpp)
else()
    message(STATUS "Using bundled SQLiteCpp")
    set(SQLITECPP_INCLUDE_SCRIPT OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
    set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
    add_subdirectory(ext/sqlitecpp)
endif()

find_package(Boost REQUIRED)

#pkg_check_modules(SDL2 sdl2 REQUIRED)
pkg_check_modules(PULSEAUDIO libpulse-simple REQUIRED)

if (PHONED_TESTING)
  find_package(GTest REQUIRED)
endif()
