#ifndef LOGPARAMETER_H
#define LOGPARAMETER_H
#include <QString>

struct LogParameter{
public:
    
    enum { LOG_NEVER=0, LOG_ONCE, LOG_EVERY, LOG_LATEST, LOG_AVERAGE, LOG_ON_CHANGE };
    
    LogParameter()=default;
    LogParameter(const QString& name, const QString& value, int a_logType = LOG_EVERY, bool updatedRecently=false)
    : a_name(name), a_value(value), updated(updatedRecently), fLogType(a_logType)
    { }
    
    void setUpdatedRecently(bool updatedRecently) {updated = updatedRecently;}
    void setName(const QString& name) {a_name = name;}
    void setValue(const QString& value) {a_value = value;}

    const QString& value() const {return a_value;}
    const QString& name() const {return a_name;}
    bool updatedRecently() const {return updated;}
    int logType() const { return fLogType; }

private:
    QString a_name, a_value;
    bool updated;
    int fLogType = LOG_NEVER;
};

#endif // LOGPARAMETER_H
