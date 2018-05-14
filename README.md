# myon_detector
software for a raspberry pi based myon detector system using a ublox gps module for timing.

Final goal:
A software solution that can use a Raspberry Pi mini computer and the Ublox m8 gps modules "timemark" feature together with 
a plastic scintillator + SciPm based detector system to measure myons up to a 50ns time accuracy. 
Therefore the software has to communicate with the ublox gps module through a serial interface using the ubx protocol. 
The high time accuracy is needed because it is planned to use multiple stations for measuring myons.
The software must be easy to use and run in background while synchronizing accumulated data with a server.


ATTENTION:  when trying to create Makefile with qmake (qt version 5.7.1 on raspbian) there will probably be an error:
            "Project ERROR: unknown module(s) in QT: serialport"
            there is missing library "libqt5serialport5-dev". To install just get it from repository with apt-get install
