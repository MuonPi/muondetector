#ifndef LOGPARAMETER_H
#define LOGPARAMETER_H
#include <QString>
struct LogParameter{
public:
    LogParameter()=default;
    LogParameter(const QString& name, const QString& value, const bool updatedRecently = false){
        updated = updatedRecently;
        a_name = name;
        a_value = value;}
    void setUpdatedRecently(bool updatedRecently) {updated = updatedRecently;}
    void setName(const QString& name) {a_name = name;}
    void setValue(const QString& value) {a_value = value;}

    const QString value() {return (const QString)a_value;}
    const QString name() {return (const QString)a_value;}
    const bool updatedRecently() {return (const bool)updated;}

private:
    QString a_name, a_value;
    bool updated;
};

#endif // LOGPARAMETER_H
