FROM debian:buster

WORKDIR /builddir/

RUN chmod 0777 /builddir/ && apt update && apt install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    cmake \
    libconfig++-dev \
    libconfig-dev \
    build-essential \
    git

RUN cd /builddir && git clone https://github.com/MuonPi/muondetector.git

ENTRYPOINT /bin/bash
