#include <QtGlobal>
#include <QTimer>
#include <logparameter.h>
#include <logengine.h>
#include <config.h>

/* g_fmt(buf,x) stores the closest decimal approximation to x in buf;
 * it suffices to declare buf
 *	char buf[32];
 */
/*
#ifdef __cplusplus
extern "C" {
#endif
 extern char *g_fmt(char *, double);
#ifdef __cplusplus
	}
#endif
*/

const int logReminderInterval = MUONPI_LOG_INTERVAL_MINUTES; // in minutes


LogEngine::LogEngine(QObject *parent) : QObject(parent)
{
    QTimer *logReminder = new QTimer(this);
    logReminder->setInterval(60*1000*logReminderInterval);
    logReminder->setSingleShot(false);
    connect(logReminder, &QTimer::timeout, this, &LogEngine::onLogInterval);
    logReminder->start();
}

LogEngine::~LogEngine()
{
}

void LogEngine::onLogParameterReceived(const LogParameter& logpar) {
//    writeToLogFile(dateStringNow()+" "+QString(log.name()+" "+log.value()+"\n"));
//    LogParameter localLog(log);
//    localLog.setUpdatedRecently(true);
    if (logpar.logType()==LogParameter::LOG_NEVER) {
		// do nothing, just return
		return;
	}
    if (logpar.logType()==LogParameter::LOG_EVERY) {
		// directly log to file since LOG_EVERY attribute is set
		// no need to store in buffer, just return after logging
//		writeToLogFile(dateStringNow()+" "+QString(log.name()+" "+log.value()+"\n"));
		emit sendLogString(QString(logpar.name()+" "+logpar.value()));
		// reset already existing entries but preserve logType attribute
		if (logData.find(logpar.name())!=logData.end()) {
			int logType=logData[logpar.name()].front().logType();
			if (logType!=LogParameter::LOG_AVERAGE) {
				logData[logpar.name()].clear();
			}
			logData[logpar.name()].push_back(LogParameter(logpar.name(),logpar.value(),logType));
			logData[logpar.name()].back().setUpdatedRecently(false);
		}
		return;
	} else {
		// save to buffer
		if (logpar.logType()==LogParameter::LOG_ONCE) {
			// don't save if a LOG_ONCE param with this name is already in buffer
			//if (logData.find(log.name())!=logData.end()) return;
		}
		logData[logpar.name()].push_back(logpar);
		logData[logpar.name()].back().setUpdatedRecently(true);
	}
}

void LogEngine::onLogInterval() {
	//emit logIntervalSignal();
	// loop over the map with all accumulated parameters since last log reminder
	// no increment here since we erase and invalidate iterators within the loop
	for (auto it=logData.begin(); it != logData.end();) {
		QString name=it.key();
		QVector<LogParameter> parVector=it.value();
		// check if name string is set but no entry exists. This should not happen
		if (parVector.isEmpty()) {
			++it;
			continue;
		}
		
		if (parVector.back().logType()==LogParameter::LOG_LATEST) {
			// easy to write only the last value to file
			//writeToLogFile(dateStringNow()+" "+name+" "+parVector.back().value()+"\n");
			emit sendLogString(name+" "+parVector.back().value());
			it=logData.erase(it);
		} else if (parVector.back().logType()==LogParameter::LOG_AVERAGE) {
			// here we loop over all values in the vector for the current parameter and do the averaging
			double sum=0.;
			bool ok=false;
			// parse last field of value string
			QString unitString=parVector.back().value().section(" ",-1,-1);
			// compare with first field
			if (unitString.compare(parVector.back().value().section(" ",0,0))==0) {
				// unit and value are identical, so there is probably no unit suffix
				// set unit to empty string
				unitString="";
			}
			// do the averaging
			for (int i=0; i<parVector.size(); i++) {
				QString valString=parVector[i].value();
				QString str=valString.section(" ",0,0);
				// convert to double with error checking
				double val=str.toDouble(&ok);
				if (!ok) break;
				sum+=val;
			}
			if (ok) {
				sum/=parVector.size();
				//writeToLogFile(dateStringNow()+" "+QString(name+" "+QString::number(sum)+" "+unitString+"\n"));
				emit sendLogString(QString(name+" "+QString::number(sum,'f',7)+" "+unitString));
				/*
				char buf[32];
				g_fmt(buf, sum);
				emit sendLogString(QString(name+" "+QString(buf)+" "+unitString));
				*/
				
			}
			it=logData.erase(it);
		} else if (parVector.back().logType()==LogParameter::LOG_ONCE) {
			// we want to log only one time per daemon lifetime || file change
			if (onceLogFlag || parVector.front().updatedRecently()) {
				//writeToLogFile(dateStringNow()+" "+name+" "+parVector.back().value()+"\n");
				emit sendLogString(name+" "+parVector.back().value());				
			}
			while (parVector.size()>2) {
				parVector.pop_front();
			}
			parVector.front().setUpdatedRecently(false);
			logData[name]=parVector;
			++it;
		} else if (parVector.back().logType()==LogParameter::LOG_ON_CHANGE) {
			// we want to log only if one value differs from the first entry
			// first entry is reference value
			if (onceLogFlag || parVector.front().updatedRecently()) {
				// log the first time anyway
				//writeToLogFile(dateStringNow()+" "+name+" "+parVector.back().value()+"\n");
				emit sendLogString(name+" "+parVector.back().value());
			} else {
				for (int i=1; i<parVector.size(); i++) {
					if (parVector[i].value().compare(parVector.front().value())!=0) {
						// found difference -> log it
						emit sendLogString(name+" "+parVector[i].value());
						//writeToLogFile(dateStringNow()+" "+name+" "+parVector[i].value()+"\n");
						parVector.replace(0,parVector[i]);
					}
				}
			}
			while (parVector.size()>1) {
				parVector.pop_back();
			}
			parVector.front().setUpdatedRecently(false);
			logData[name]=parVector;
			++it;
		} else ++it;
	}
	onceLogFlag=false;
}
