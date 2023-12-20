#!/bin/sh
# From https://github.com/scylladb/seastar/blob/master/install-dependencies.sh
apt-get install -y ninja-build \
    ragel \
    libhwloc-dev \
    libnuma-dev \
    libpciaccess-dev \
    libcrypto++-dev \
    libboost-all-dev \
    libxml2-dev \
    xfslibs-dev \
    libgnutls28-dev \
    liblz4-dev \
    libsctp-dev \
    liburing-dev \
    gcc \
    make \
    meson \
    python3 \
    python3-pyelftools \
    systemtap-sdt-dev \
    libtool \
    cmake \
    libyaml-cpp-dev \
    libc-ares-dev \
    stow \
    g++ \
    libfmt-dev \
    diffutils \
    valgrind \
    doxygen \
    openssl \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler
    
apt-get install -y \
                    git \
                    curl \
                    gdb \
                    unzip \
                    tar
