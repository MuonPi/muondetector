FROM debian:buster

WORKDIR /builddir/

RUN apt update && apt install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    cmake \
    libconfig++-dev \
    libconfig-dev \
    build-essential \
    git \
    libqt5serialport5-dev \
    libqt5svg5-dev \
    qtdeclarative5-dev \
    libpigpiod-if-dev \
    libcrypto++-dev \


RUN cd /builddir && git clone https://github.com/MuonPi/muondetector.git

ENTRYPOINT /bin/bash
