#include "hateeprom.h"
#include <QtGlobal>

HatEeprom::HatEeprom(QObject *parent) : QObject(parent)
{

}

HatEeprom::init(){

    // try to find out on which hardware version we are running
    // for this to work, we have to initialize and read the eeprom first
    // EEPROM 24AA02 type
    eep = new EEPROM24AA02();
    calib = new ShowerDetectorCalib(eep);
    if (eep->devicePresent()) {
        calib->readFromEeprom();
        if (verbose>2) {
            qInfo()<<"EEP device is present";
//			readEeprom();
//			calib->readFromEeprom();

            // Caution, only for debugging. This code snippet writes a test sequence into the eeprom
            if (1==0) {
                uint8_t buf[256];
                for (int i=0; i<256; i++) buf[i]=i;
                if (!eep->writeBytes(0, 256, buf)) cerr<<"error: write to eeprom failed!"<<endl;
                if (verbose>2) cout<<"eep write took "<<eep->getLastTimeInterval()<<" ms"<<endl;
                readEeprom();
            }
            if (1==1) {
                calib->printCalibList();
            }
        }
        uint64_t id=calib->getSerialID();
        QString hwIdStr="0x"+QString::number(id,16);
//		qInfo()<<"eep unique ID: 0x"<<hex<<id<<dec;
        qInfo()<<"EEP unique ID:"<<hwIdStr;


        //if (calib->isValid()) {

        //}
    } else {
        qCritical()<<"eeprom device NOT present!";
    }
}
