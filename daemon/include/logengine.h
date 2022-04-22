#ifndef LOGENGINE_H
#define LOGENGINE_H
#include "logparameter.h"
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <config.h>

class LogEngine : public QObject {
    Q_OBJECT

public:
    LogEngine(QObject* parent = nullptr);
    ~LogEngine();

signals:
    void sendLogString(const QString& str);
    void logIntervalSignal();

public slots:
    void onLogParameterReceived(const LogParameter& logpar);
    void onLogInterval();
    void onOnceLogTrigger() { onceLogFlag = true; }

private:
    QMap<QString, QVector<LogParameter>> logData;
    bool onceLogFlag = true;
};

#endif // LOGENGINE_H
