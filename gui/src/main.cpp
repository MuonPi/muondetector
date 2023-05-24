#include <QApplication>
#include <QCommandLineParser>
#include <mainwindow.h>

#include "config.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("muondetector-gui");
    QCoreApplication::setApplicationVersion(QString::fromStdString(MuonPi::Version::software.string()));
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(app);

    MainWindow w;
    w.show();

    return app.exec();
}
