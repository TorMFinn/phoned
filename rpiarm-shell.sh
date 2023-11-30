#!/bin/bash

# Exit if any error is encountered.
set -e

# First, build the dev environment.
docker build -t phoned_build:rpi_bullseye32 -f docker/buildimg-rpi.dockerfile docker

# Launch the build shell
docker run \
    --rm \
    -it \
    --env PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig \
    -v $PWD:/workspace \
    "phoned_build:rpi_bullseye32" /bin/bash