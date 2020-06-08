FROM ubuntu:18.04

# first of all upgrade
RUN apt-get update && apt-get upgrade -yqq

# install build tools
RUN apt-get update && apt-get install -yqq build-essential autoconf gcc-8 g++-8 git libtool wget ninja-build

# install CMake 3.15 binary from Kitware (apt provides 3.10)
RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.4/cmake-3.15.4-Linux-x86_64.sh -O /usr/local/install_cmake.sh
RUN chmod +x /usr/local/install_cmake.sh
RUN /usr/local/install_cmake.sh --skip-license
RUN rm /usr/local/install_cmake.sh

# install video capture libraries
RUN apt-get update -yqq && apt-get install -yqq libavcodec-dev libavutil-dev libavfilter-dev libavformat-dev libswresample-dev libdc1394-22-dev libv4l-dev

# install video rendering libraries
RUN apt-get update -yqq && apt-get install -yqq libsdl2-dev libsdl2-2.0-0 libglu1-mesa-dev libglu1-mesa libgles2-mesa-dev

# build puredata from source 
COPY scripts/build-puredata.sh /tmp/build-puredata.sh
RUN /tmp/build-puredata.sh
RUN rm /tmp/build-puredata.sh

# install Gem dependencies
RUN apt-get update -yqq && apt-get install -yqq libglew-dev

# build Gem from source 
COPY scripts/build-gem.sh /tmp/build-gem.sh
RUN /tmp/build-gem.sh
RUN rm /tmp/build-gem.sh

# install tcl/tk
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -yqq tcl tk

# install curl
RUN apt-get update && apt-get install -yqq curl

# install curl
RUN apt-get update && apt-get install -yqq libmpv1 libmpv-dev

# clean apt cache and remove apt package lists
RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*

