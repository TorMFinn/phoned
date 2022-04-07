option(PHONED_TESTING "Build phoned unit tests" OFF)
option(PHONED_SYSTEM_SQLITECPP "Use system installed SQLiteCPP" OFF)

# Enable testing if user has set option to true
if (PHONED_TESTING)
  enable_testing()
endif()

# Output files to bin and lib directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
