#include "logengine.h"
#include "logparameter.h"
#include <QDateTime>
#include <QTimer>
#include <QtGlobal>
#include <config.h>

static QString dateStringNow()
{
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd_hh-mm-ss");
}

LogEngine::LogEngine(QObject* parent)
    : QObject(parent)
{
    QTimer* logReminder = new QTimer(this);
    logReminder->setInterval(1000 * MuonPi::Config::Log::interval.count());
    logReminder->setSingleShot(false);
    connect(logReminder, &QTimer::timeout, this, &LogEngine::onLogInterval);
    logReminder->start();
}

LogEngine::~LogEngine()
{
}

void LogEngine::onLogParameterReceived(const LogParameter& logpar)
{
    if (logpar.logType() == LogParameter::LOG_NEVER) {
        return;
    }
    if (logpar.logType() == LogParameter::LOG_EVERY) {
        // directly log to file since LOG_EVERY attribute is set
        // no need to store in buffer, just return after logging
        emit sendLogString(dateStringNow() + " " + QString(logpar.name() + " " + logpar.value()));
        // reset already existing entries but preserve logType attribute
        if (logData.find(logpar.name()) != logData.end()) {
            int logType = logData[logpar.name()].front().logType();
            if (logType != LogParameter::LOG_AVERAGE) {
                logData[logpar.name()].clear();
            }
            logData[logpar.name()].push_back(LogParameter(logpar.name(), logpar.value(), logType));
            logData[logpar.name()].back().setUpdatedRecently(false);
        }
        return;
    } else {
        // save to buffer
        if (logpar.logType() == LogParameter::LOG_ONCE) {
            // don't save if a LOG_ONCE param with this name is already in buffer
        }
        logData[logpar.name()].push_back(logpar);
        logData[logpar.name()].back().setUpdatedRecently(true);
    }
}

void LogEngine::onLogInterval()
{
    emit logIntervalSignal();

    // Use one timestamp for writing all parameters
    // Otherwise log messages can spread over multiple seconds, making data import challenging
    auto ts = dateStringNow();

    // loop over the map with all accumulated parameters since last log reminder
    // no increment here since we erase and invalidate iterators within the loop
    for (auto it = logData.begin(); it != logData.end();) {
        QString name = it.key();
        QVector<LogParameter> parVector = it.value();
        // check if name string is set but no entry exists. This should not happen
        if (parVector.isEmpty()) {
            ++it;
            continue;
        }

        if (parVector.back().logType() == LogParameter::LOG_LATEST) {
            // easy to write only the last value to file
            emit sendLogString(ts + " " + name + " " + parVector.back().value());
            it = logData.erase(it);
        } else if (parVector.back().logType() == LogParameter::LOG_AVERAGE) {
            // here we loop over all values in the vector for the current parameter and do the averaging
            double sum = 0.;
            bool ok = false;
            // parse last field of value string
            QString unitString = parVector.back().value().section(" ", -1, -1);
            // compare with first field
            if (unitString.compare(parVector.back().value().section(" ", 0, 0)) == 0) {
                // unit and value are identical, so there is probably no unit suffix
                // set unit to empty string
                unitString = "";
            }
            // do the averaging
            for (int i = 0; i < parVector.size(); i++) {
                QString valString = parVector[i].value();
                QString str = valString.section(" ", 0, 0);
                // convert to double with error checking
                double val = str.toDouble(&ok);
                if (!ok)
                    break;
                sum += val;
            }
            if (ok) {
                sum /= parVector.size();
                emit sendLogString(ts + " " + QString(name + " " + QString::number(sum, 'f', 7) + " " + unitString));
            }
            it = logData.erase(it);
        } else if (parVector.back().logType() == LogParameter::LOG_ONCE) {
            // we want to log only one time per daemon lifetime || file change
            if (onceLogFlag || parVector.front().updatedRecently()) {
                emit sendLogString(ts + " " + name + " " + parVector.back().value());
            }
            while (parVector.size() > 2) {
                parVector.pop_front();
            }
            parVector.front().setUpdatedRecently(false);
            logData[name] = parVector;
            ++it;
        } else if (parVector.back().logType() == LogParameter::LOG_ON_CHANGE) {
            // we want to log only if one value differs from the first entry
            // first entry is reference value
            if (onceLogFlag || parVector.front().updatedRecently()) {
                // log the first time anyway
                emit sendLogString(ts + " " + name + " " + parVector.back().value());
            } else {
                for (int i = 1; i < parVector.size(); i++) {
                    if (parVector[i].value().compare(parVector.front().value()) != 0) {
                        // found difference -> log it
                        emit sendLogString(ts + " " + name + " " + parVector[i].value());
                        parVector.replace(0, parVector[i]);
                    }
                }
            }
            while (parVector.size() > 1) {
                parVector.pop_back();
            }
            parVector.front().setUpdatedRecently(false);
            logData[name] = parVector;
            ++it;
        } else
            ++it;
    }
    onceLogFlag = false;
}
