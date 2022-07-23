#ifndef MAP_H
#define MAP_H

#include <QWidget>
#include <ublox_structs.h>

namespace Ui {
class Map;
}

class Map : public QWidget {
    Q_OBJECT

public:
    explicit Map(QWidget* parent = nullptr);
    ~Map();

public slots:
    void onGeodeticPosReceived(GeodeticPos pos);
    void onUiEnabledStateChange(bool connected);

private:
    QObject* mapComponent = nullptr;
    Ui::Map* mapUi;
};
#endif // MAP_H
