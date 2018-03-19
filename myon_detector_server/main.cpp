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

    // process the actual command line arguments given by the user
    parser.process(a);
    int verbose = 0;
    if (verbose > 2){
        cout << "int main running in thread "
             << QCoreApplication::instance()->thread() << endl;
    }
        TcpServer server(verbose);
        return a.exec();
}
