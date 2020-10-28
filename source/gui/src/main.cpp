#include <mainwindow.h>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("muondetector-gui");
    QCoreApplication::setApplicationVersion(VERSION_STRING);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(a);

    MainWindow w;
    w.show();

    return a.exec();
}
