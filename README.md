# MuonPi muondetector Project

<p align="center">
  <strong>Raspberry Pi detector software for time-correlated cosmic-ray muon measurements.</strong>
</p>

<p align="center">
  <a href="https://www.muonpi.org/">MuonPi.org</a>
  &nbsp;|&nbsp;
  <a href="https://github.com/MuonPi/muondetector/releases">Releases</a>
  &nbsp;|&nbsp;
  <a href="https://wiki.muonpi.org">Wiki</a>
  &nbsp;|&nbsp;
  <a href="LICENSE">License</a>
</p>

<p align="center">
  <a href="https://github.com/MuonPi/muondetector/actions/workflows/ci.yml">
    <img alt="CI" src="https://img.shields.io/github/actions/workflow/status/MuonPi/muondetector/ci.yml?branch=main&label=CI&style=flat-square">
  </a>
  <a href="https://en.cppreference.com/w/cpp/20">
    <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square">
  </a>
  <a href="https://www.qt.io/product/qt6">
    <img alt="Qt 6" src="https://img.shields.io/badge/Qt-6-41CD52?style=flat-square">
  </a>
  <a href="https://cmake.org/">
    <img alt="CMake" src="https://img.shields.io/badge/CMake-build-064F8C?style=flat-square">
  </a>
  <a href="https://capnproto.org/">
    <img alt="Cap'n Proto" src="https://img.shields.io/badge/Cap'n%20Proto-protocol-F6C915?style=flat-square">
  </a>
  <a href="https://mqtt.org/">
    <img alt="MQTT" src="https://img.shields.io/badge/MQTT-data%20upload-660066?style=flat-square">
  </a>
  <a href="https://discord.gg/mrgAUFxwEj">
    <img alt="Discord" src="https://img.shields.io/badge/Discord-join%20chat-5865F2?style=flat-square&logo=discord&logoColor=white">
  </a>
  <a href="https://www.raspberrypi.com/">
    <img alt="Raspberry Pi" src="https://img.shields.io/badge/Raspberry%20Pi-supported-C51A4A?style=flat-square">
  </a>
  <a href="LICENSE">
    <img alt="License LGPL-3.0-or-later" src="https://img.shields.io/badge/license-LGPL--3.0--or--later-blue?style=flat-square">
  </a>
</p>

MuonPi muondetector is the software stack for operating a Raspberry Pi based
muon detector with a u-blox GNSS receiver, detector front-end electronics and
networked data upload. It timestamps detector events with GNSS timing, exposes
live control and monitoring through a Qt GUI, and can store or publish
measurements for later correlation between independent detector stations.

The current generation is the v3 software line: a rebuilt, component-based
daemon with configuration-driven hardware, sources, sinks and maintenance
tasks.

## At a Glance

| Area | What it provides |
| --- | --- |
| Timing | u-blox GNSS handling, UBX parsing and timemark/event processing |
| Hardware | GPIO, I2C and serial access for detector and environmental devices |
| Data | Cap'n Proto based TCP protocol, file storage and MQTT publishing |
| Control | Qt 6 GUI for monitoring, calibration and remote detector control |
| Deployment | Debian packages, systemd service files and configuration templates |

## Repository Layout

```text
daemon/                 muondetector-daemon, hardware drivers and service files
credentials_utility/    muondetector-login for MQTT credential storage
gui/                    Qt 6 desktop GUI
library/                shared protocol, data structures and TCP client code
tests/                  CTest-based TCP protocol and server tests
tools/                  helper tools and protocol generation scripts
cmake/                  build modules and cross-compilation toolchains
```

## Raspberry Pi Preparation

Before running the daemon on detector hardware, enable the Raspberry Pi
interfaces used by the detector electronics. This is necessary because the
software uses the builtin hardware I2C and serial interface.
Be aware that serial login shell "on" **will** break the functionality of the muondetector software.
SPI is usually not required but certain peripherals may use it and we usually turn it on by default:

```bash
sudo raspi-config
```

Enable:
```
- serial hardware, with the serial login shell disabled
- I2C
- SPI, if required by the connected detector hardware
```

## Install Packages

For normal detector stations, install the released Debian packages instead of
building by hand. Download the latest release from GitHub, or follow the guided
installation notes on the MuonPi website:

- Website and project documentation: [MuonPi.org](https://www.muonpi.org/)
- Binary packages and release notes: [GitHub Releases](https://github.com/MuonPi/muondetector/releases)

The package installation sets up the daemon, configuration files and systemd
integration expected on a Raspberry Pi detector station.

## Build from Source

The commands below build the repository from a clean checkout on Debian,
Raspberry Pi OS or Ubuntu-like systems. They install the development packages
needed for the daemon, GUI, protocol generator and optional test targets.

```bash
sudo apt update
sudo apt install \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config \
  python3 \
  file \
  gzip \
  capnproto \
  libcapnp-dev \
  libboost-dev \
  libconfig++-dev \
  libglib2.0-dev \
  libmosquitto-dev \
  libgpiod-dev \
  qt6-base-dev \
  qt6-base-dev-tools \
  qt6-declarative-dev \
  qt6-positioning-dev \
  libqt6svg6-dev \
  libqt6opengl6-dev \
  qml6-module-qtlocation \
  qml6-module-qtpositioning \
  qml6-module-qtquick2 \
  qml6-module-qtquick-controls \
  qml6-module-qtquick-controls2 \
  qml6-module-qtquick-layouts \
  qml6-module-qtquick-templates2 \
  libqwt-qt6-dev
```

Clone and configure the build:

```bash
git clone https://github.com/MuonPi/muondetector.git
cd muondetector
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DMUONDETECTOR_BUILD_GUI=ON \
  -DMUONDETECTOR_BUILD_DAEMON=ON \
  -DMUONDETECTOR_BUILD_HARDWARE_LIB=ON
```

Compile everything:

```bash
cmake --build build --parallel
```

The built binaries are written below:

```text
build/output/bin/muondetector-daemon
build/output/bin/muondetector-login
build/output/bin/muondetector-gui
```

To build Debian packages on a Raspberry Pi or in a suitable packaging
environment:

```bash
cmake --build build --target package --parallel
ls build/output/packages
```

## Useful Build Options

| Option | Default | Purpose |
| --- | --- | --- |
| `MUONDETECTOR_BUILD_GUI` | `ON` | Build the Qt 6 GUI |
| `MUONDETECTOR_BUILD_DAEMON` | `ON` | Build the detector daemon and login utility |
| `MUONDETECTOR_BUILD_HARDWARE_LIB` | `ON` | Build the hardware support library |
| `MUONDETECTOR_BUILD_TESTS` | `OFF` | Build CTest integration tests |
| `MUONDETECTOR_BUILD_TCP_DEBUG_CLIENT` | `OFF` | Build the standalone TCP debug client |
| `MUONDETECTOR_BUILD_TCP_DEBUG_SERVER` | `OFF` | Build the TCP test server |
| `PACKAGING_MODE` | `OFF` | Disable GUI deploy steps for package builds |

For a daemon-only build:

```bash
cmake -S . -B build-daemon -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DMUONDETECTOR_BUILD_GUI=OFF \
  -DMUONDETECTOR_BUILD_DAEMON=ON \
  -DMUONDETECTOR_BUILD_HARDWARE_LIB=ON
cmake --build build-daemon --parallel
```

For tests:

```bash
cmake -S . -B build-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMUONDETECTOR_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

## Configuration and Runtime

Installed packages place configuration below `/etc/muondetector/`:

```text
/etc/muondetector/muondetector.conf
/etc/muondetector/hardware.conf
/etc/muondetector/components.conf
/etc/muondetector/settings.conf
```

Store MQTT credentials before starting the daemon:

```bash
sudo muondetector-login store
```

Start and inspect the systemd service:

```bash
sudo systemctl enable muondetector-daemon.service
sudo systemctl start muondetector-daemon.service
systemctl status muondetector-daemon.service
```

Launch the GUI from the Raspberry Pi or another networked computer:

```bash
muondetector-gui
```

The daemon and login utility also install manpages:

```bash
man muondetector-daemon
man muondetector-login
```

## Documentation Links

- [MuonPi.org](https://www.muonpi.org/) is the best starting point for detector setup and package installation.
- [GitHub Releases](https://github.com/MuonPi/muondetector/releases) contains current binaries and release notes.
- [Project Wiki](https://wiki.muonpi.org) contains background material and extended documentation.

## License

MuonPi muondetector is licensed under the GNU Lesser General Public License,
version 3 or later. See [LICENSE](LICENSE) for the full license text.
