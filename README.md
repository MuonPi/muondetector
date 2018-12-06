# muondetector
software for a [raspberry pi based muon detector system](https://balu.physik.uni-giessen.de:8081/mediawiki/index.php) using a ublox gps module for timing.

Final goal:
A software solution that can use a Raspberry Pi mini computer and the Ublox m8 gps modules "timemark" feature together with 
a plastic scintillator + SiPM based detector system to measure muons up to a 20ns time accuracy. 
Therefore the software has to communicate with the ublox gps module through a serial interface using the ubx protocol. 
The high time accuracy is needed because it is planned to use multiple stations for measuring muons.
The software must be easy to use and run in background while synchronizing accumulated data with a server.

You can download the binaries (.deb packages inside of "install ready" folder) and just install them with "sudo apt install <'filename'>"
or on debian jessie "sudo gdebi <'filename'>". You will have to install the "libmuondetector-shared" library first before you can install
the gui or the daemon.

ATTENTION:  
            when trying to create Makefile with qmake (qt version 5.7.1 on raspbian) there will probably be errors.
            To install just get it from repository with apt-get install:
            
            "Project ERROR: unknown module(s) in QT: serialport"
            there is missing library "libqt5serialport5-dev".

            "Project ERROR: unknown module(s) in QT: quickwidgets"
            there is missing library "qtdeclarative5-dev".
            
            "Project ERROR: unknown module(s) in QT: svg"
            there is missing library "libqt5svg5-dev".
            
            You also need to install "libqwt-qt5-dev" or "libqwt-dev".
            
            You may also install "lftp" for uploading acquired data to our server.
