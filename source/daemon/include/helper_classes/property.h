#ifndef PROPERTY_H
#define PROPERTY_H

#include <QString>
#include <QVariant>
#include <QMetaType>
class Property {
public:
    Property()=default;

    template <typename T>
    Property(const T& val) : name(""), unit ("") {
        typeId = qMetaTypeId<T>();
        if (typeId==QMetaType::UnknownType) {
            typeId = qRegisterMetaType<T>();
        }
        value=QVariant(val);
        updated=true;
    }

    template <typename T>
    Property(const QString& a_name, const T& val, const QString& a_unit = "")
        : name(a_name), unit(a_unit)
    {
        typeId = qMetaTypeId<T>();
        if (typeId==QMetaType::UnknownType) {
            typeId = qRegisterMetaType<T>();
        }
        value=QVariant(val);
        updated=true;
    }

    Property(const Property& prop) = default;
    Property& operator=(const Property& prop) {
        name=prop.name;
        unit=prop.unit;
        value=prop.value;
        typeId=prop.typeId;
        updated=prop.updated;
        return *this;
    }

    Property& operator=(const QVariant& val) {
        value = val;
        //lastUpdate = std::chrono::system_clock::now();
        updated = true;
        return *this;
    }

    const QVariant& operator()() {
        updated = false;
        return value;
    }

    bool isUpdated() const { return updated; }
//    QMetaType::Type type() const { return static_cast<QMetaType::Type>(value.type()); }
    int type() const { return typeId; }

    QString name="";
    QString unit="";

private:
    QVariant value;
    bool updated=false;
    int typeId=0;
};

#endif // PROPERTY_H
