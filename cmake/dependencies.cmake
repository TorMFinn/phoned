# List of dependencies required for the project

include(FindPkgConfig)

find_package(SQLiteCpp REQUIRED)
find_package(Boost REQUIRED)

pkg_check_modules(SDL2 sdl2 REQUIRED)

if (PHONED_TESTING)
  find_package(GTest REQUIRED)
endif()
