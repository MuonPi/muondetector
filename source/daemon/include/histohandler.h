#ifndef HISTOHANDLER_H
#define HISTOHANDLER_H

#include <QObject>

class HistoHandler : public QObject
{
    Q_OBJECT
public:
    explicit HistoHandler(QObject *parent = nullptr);

signals:

};

#endif // HISTOHANDLER_H
