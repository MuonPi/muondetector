#include "unix_sig_handler_daemon.h"
#include "unistd.h"

int unix_sig_handler_daemon::sighupFd[2];
int unix_sig_handler_daemon::sigtermFd[2];
int unix_sig_handler_daemon::sigIntFd[2];

unix_sig_handler_daemon::unix_sig_handler_daemon(QObject *parent)
    : QObject(parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
       qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
       qFatal("Couldn't create TERM socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFd))
       qFatal("Couldn't create INT socketpair");

    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
    snInt = new QSocketNotifier(sigIntFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
}
unix_sig_handler_daemon::~unix_sig_handler_daemon(){
    delete snHup;
    delete snTerm;
    delete snInt;
}

void unix_sig_handler_daemon::hupSignalHandler(int)
{
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void unix_sig_handler_daemon::termSignalHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}
void unix_sig_handler_daemon::intSignalHandler(int)
{
    char a = 1;
    ::write(sigIntFd[0], &a, sizeof(a));
}
void unix_sig_handler_daemon::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit myTermSignal();
    snTerm->setEnabled(true);
}

void unix_sig_handler_daemon::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit myHupSignal();
    snHup->setEnabled(true);
}

void unix_sig_handler_daemon::handleSigInt()
{
    snInt->setEnabled(false);
    char tmp;
    ::read(sigIntFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit myIntSignal();
    snInt->setEnabled(true);
}
