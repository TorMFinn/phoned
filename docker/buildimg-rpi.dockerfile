FROM debian:bullseye

# Add armhf architecture
RUN dpkg --add-architecture armhf

# Install base dependencies
RUN apt-get update && apt-get install -y \
  wget \
  build-essential \
  xz-utils \
  sudo

# Setup a devuser with sudo access.
RUN useradd -ms /bin/bash devuser && echo "devuser:devuser" | chpasswd && adduser devuser sudo

USER devuser

# Fetch the latest version of ziglang
WORKDIR /home/devuser
RUN wget https://ziglang.org/builds/zig-linux-x86_64-0.12.0-dev.1643+91329ce94.tar.xz -O zig.tar.xz
RUN mkdir zig
RUN tar xf zig.tar.xz -C zig --strip-components=1 && rm zig.tar.xz

ENV PATH=${PATH}:/home/devuser/zig

USER root

# Setup dependencies for phoned.
RUN apt-get update && apt-get install --no-install-recommends -y \
  libasound2-dev \
  libpulse-dev \
  pkg-config 

USER devuser