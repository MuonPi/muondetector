FROM debian:buster

WORKDIR /builddir/

RUN apt update

RUN apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf cmake libconfig++-dev libconfig-dev build-essential git -y
