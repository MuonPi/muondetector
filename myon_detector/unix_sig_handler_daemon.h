#ifndef UNIX_SIG_HANDLER_DAEMON_H
#define UNIX_SIG_HANDLER_DAEMON_H

#include <QObject>
#include <QSocketNotifier>
#include <signal.h>
#include <sys/socket.h>

class unix_sig_handler_daemon : public QObject
{
    Q_OBJECT

public:
    unix_sig_handler_daemon(QObject *parent = 0);
    ~unix_sig_handler_daemon();
    // Unix signal handlers.
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);
    static void intSignalHandler(int unused);

signals:
    void myTermSignal();
    void myHupSignal();
    void myIntSignal();

public slots:
    // Qt signal handlers.
    void handleSigHup();
    void handleSigTerm();
    void handleSigInt();

private:
    static int sighupFd[2];
    static int sigtermFd[2];
    static int sigIntFd[2];

    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;
    QSocketNotifier *snInt;
};
static int setup_unix_signal_handlers();

#endif // UNIX_SIG_HANDLER_DAEMON_H
