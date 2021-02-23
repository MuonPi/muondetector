#ifndef HATEEPROM_H
#define HATEEPROM_H

#include <QObject>

class HatEeprom : public QObject
{
    Q_OBJECT
public:
    explicit HatEeprom(QObject *parent = nullptr);

signals:

};

#endif // HATEEPROM_H
