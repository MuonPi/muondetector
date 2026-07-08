#include "config.h"

#include <QApplication>
#include <QCommandLineParser>
#include <boost/asio.hpp>
#include <mainwindow.h>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("muondetector-gui");
    QCoreApplication::setApplicationVersion(
        QString::fromStdString(MuonPi::Version::software.string()));
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(app);

    auto io = std::make_shared<boost::asio::io_context>();
    auto guard = boost::asio::make_work_guard(*io);
    auto ioThread = std::thread([&io]() { io->run(); });

    int result = 0;
    {
        MainWindow w(io);
        w.show();
        result = app.exec();
    }
    io->stop();
    ioThread.join();
    return result;
}
