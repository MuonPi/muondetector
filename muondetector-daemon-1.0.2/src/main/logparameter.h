#ifndef LOGPARAMETER_H
#define LOGPARAMETER_H
#include <QString>
struct LogParameter{
public:
    LogParameter()=default;
    LogParameter(const QString& name, const QString& value, bool updatedRecently = false){
        updated = updatedRecently;
        a_name = name;
        a_value = value;}
    void setUpdatedRecently(bool updatedRecently) {updated = updatedRecently;}
    void setName(const QString& name) {a_name = name;}
    void setValue(const QString& value) {a_value = value;}

    const QString& value() const {return a_value;}
    const QString& name() const {return a_name;}
    bool updatedRecently() const {return updated;}

private:
    QString a_name, a_value;
    bool updated;
};

#endif // LOGPARAMETER_H
