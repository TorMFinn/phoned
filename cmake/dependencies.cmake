# List of dependencies required for the project

find_library(pulseaudio_LIBRARY pulse REQUIRED)
find_library(pulseaudio_simple_LIBRARY pulse-simple REQUIRED)

set(pulseaudio_LIBRARIES ${pulseaudio_LIBRARY} ${pulseaudio_simple_LIBRARY})
