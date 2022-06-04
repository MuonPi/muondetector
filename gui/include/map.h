#ifndef MAP_H
#define MAP_H

#include <QWidget>
#include <ublox_structs.h>

struct PositionModeConfig;

namespace Ui {
class Map;
}

class Map : public QWidget {
    Q_OBJECT

public:
    explicit Map(QWidget* parent = nullptr);
    ~Map();

public slots:
    void onGeodeticPosReceived(const GnssPosStruct& pos);
    void onPosConfigReceived(const PositionModeConfig& pos);
    void onUiEnabledStateChange(bool connected);

private:
    QObject* mapComponent = nullptr;
    Ui::Map* mapUi;
};
#endif // MAP_H
