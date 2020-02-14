#include <QCoreApplication>
#include <pigpiodhandler.h>
#include <tdc7200.h>
#include <iostream>
#include <QVector>
#include <QDebug>

const unsigned int intb_pin = 20;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<std::string>("std::string");
    PigpiodHandler pighandler(QVector<unsigned int>{intb_pin}, 61035, 0);
    TDC7200 tdc(intb_pin);
    pighandler.connect(&pighandler, &PigpiodHandler::spiData, &tdc, &TDC7200::onDataReceived);
    pighandler.connect(&tdc, &TDC7200::readData, &pighandler, &PigpiodHandler::readSpi);
    pighandler.connect(&tdc, &TDC7200::writeData, &pighandler, &PigpiodHandler::writeSpi);
    pighandler.connect(&pighandler, &PigpiodHandler::signal, &tdc, &TDC7200::onDataAvailable);
    tdc.initialise();
    tdc.connect(&tdc, &TDC7200::timeMeas, [](QVector<double> timings){
        std::cout << timings.at(0) << std::endl;
    });

    //std::cout << "press a key to start tdc measurement" << std::endl;
    //std::cin.ignore();
    tdc.startMeas();
    return a.exec();
}
