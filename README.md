# muondetector
Software for a [Raspberry Pi based muon detector system](https://balu.physik.uni-giessen.de:8081/mediawiki/index.php) using a Ublox GPS module for precise timing.

ABSTRACT

The goal work is the create a software solution that can use a Raspberry Pi mini computer and the Ublox M8 GPS module's "timemark" feature together with a plastic scintillator + SiPM-based detector system to measure muons up to a 20ns time accuracy. Therefore, the software has to communicate with the Ublox GPS module through a serial interface using the ubx protocol. The good time accuracy is needed for correlating several independent detector units for the reconstruction of atmospheric muon showers resulting from ultra high-energy cosmic particles impinging on the earth's atmosphere. The software must be easy-to-use and runs in the background while synchronizing accumulated data with a central server.

DOWNLOAD

The latest binaries can be found as Debian packages with patch-notes and other release specific information in the "Releases" folder.

INSTALLATION (example for version 1.0.3)

Installing .deb Debian packages: "sudo apt install <./filename>" or on debian jessie "sudo gdebi <'filename'>". An internet connection might be needed for installing additional dependencies.
On your Raspberry Pi:
1. Install "libmuondetector-shared_1.0.3-raspbian.deb" 
2. Install "muondetector-daemon_1.0.3-raspbian.deb"
Depending on the device of you choice, install the GUI for controlling the software via network connection or on the Raspberry Pi itself:
3. Install 
      (a) "muondetector-gui_1.0.3-raspbian.deb" on a Raspberry Pi
      (b) "muondetector-gui_1.0.3-ubuntu_bionic-x64.deb" on a Ubuntu 18.xx machine 
      (c) "muondetector-gui_1.0.3-windows-x64.zip" on a 64-bit Windows machine 
4. Enable serial connections on the Raspberry Pi (either use (a) or (b)):
      (a) Use "sudo raspi-config" > Interfacing options > Serial > Login Shell "no" > Enable serial port hardware "yes"
      (b) Manually add "enable_uart=1" to /boot/config.txt

TROUBLESHOOTING AND DEPENDENCIES:  
When trying to create a Makefile with qmake (qt version 5.7.1 on raspbian) there will probably be errors. To install just get it from repository with apt-get install:
      - "Project ERROR: unknown module(s) in QT: serialport" there is a missing library "libqt5serialport5-dev".
      - "Project ERROR: unknown module(s) in QT: quickwidgets" there is a missing library "qtdeclarative5-dev".
      - "Project ERROR: unknown module(s) in QT: svg" there is a missing library "libqt5svg5-dev".
      - You also need to install "libcrypto++-dev"
      - You also need to install "libqwt-qt5-dev" or "libqwt-dev".
      - You may also install "lftp" for uploading acquired data to our server.

RUNNING THE SOFTWARE
On your Raspberry Pi, do:
1. "sudo pigpiod -s 1" for setting the sampling rate to 1
2. start the daemon with "muondetector_daemon <device> [options]" where device is your serial interface (either "/dev/ttyS0" or "/dev/ttyAMA0") and the options can be viewed by adding -h. It is recommended to use the -c option.
On your network device or on your Raspberry Pi: 
3. Start the gui with "muondetector_gui" and have fun

