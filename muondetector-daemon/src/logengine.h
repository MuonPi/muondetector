#ifndef LOGENGINE_H
#define LOGENGINE_H
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <logparameter.h>

class LogEngine : public QObject
{
	Q_OBJECT

public:
    LogEngine(QObject *parent = nullptr);
    ~LogEngine();
	
signals:
	void sendLogString(const QString& str);
	void logIntervalSignal();
	
public slots:
	void onLogParameterReceived(const LogParameter& logpar);
	void onLogInterval();
	void onOnceLogTrigger() { onceLogFlag=true; }

private:
	QMap<QString, QVector<LogParameter> > logData;
	bool onceLogFlag=true;
};

#endif // LOGENGINE_H
