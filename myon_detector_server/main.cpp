#include <QCoreApplication>
#include <QCommandLineParser>
#include "custom_io_operators.h"
#include "tcpserver.h"

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    QCoreApplication::setApplicationName("myon_detector_server");
    QCoreApplication::setApplicationVersion("1.0");

    // command line input management
    QCommandLineParser parser;
    parser.setApplicationDescription("Server program for U-Blox GPS polling and configuration program\n"
        "2018 HG Zaunick <zaunick@exp2.physik.uni-giessen.de>\n"
        "with added tcp implementation for synchronising "
        "data with a central server");
    parser.addHelpOption();
    parser.addVersionOption();

    // verbosity option
    QCommandLineOption verbosityOption(QStringList() << "e" << "verbose",
        QCoreApplication::translate("main", "set verbosity level\n"
            "3 is max"),
        QCoreApplication::translate("main", "verbosity"));
    parser.addOption(verbosityOption);

    // ip option
    QCommandLineOption ipOption(QStringList() << "ip" << "address",
        QCoreApplication::translate("main", "set server ip address"),
        QCoreApplication::translate("main", "ipAddress"));
    parser.addOption(ipOption);

    // port option
    QCommandLineOption portOption(QStringList() << "p" << "port",
        QCoreApplication::translate("main", "set server port"),
        QCoreApplication::translate("main", "port"));
    parser.addOption(portOption);

    // process the actual command line arguments given by the user
    parser.process(a);
    bool ok;
    int verbose = 0;
    if (parser.isSet(verbosityOption)) {
        verbose = parser.value(verbosityOption).toInt(&ok);
        if (!ok) {
            verbose = 0;
            cout << "wrong input verbosity level" << endl;
        }
    }
    quint16 port = 0;
    if (parser.isSet(portOption)){
        port = parser.value(portOption).toInt(&ok);
        if (!ok) {
            port = 0;
            cout << "wrong input port (maybe not an integer)" << endl;
        }
    }
    QString ipAddress;
    if (parser.isSet(ipOption)){
        ipAddress = parser.value(ipOption);
        if (!QHostAddress(ipAddress).toIPv4Address()){
            if (ipAddress != "localhost" && ipAddress != "local"){
                ipAddress = "";
                cout << "wrong input ipAddress, not an ipv4address" << endl;
            }
        }
    }
    if (verbose > 2){
        cout << "int main running in thread "
             << QCoreApplication::instance()->thread() << endl;
    }
    TcpServer server(ipAddress, port, verbose);
    return a.exec();
}
