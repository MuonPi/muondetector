#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSsl>
#include <cmath>
#include <map.h>

#include <ui_map.h>

Map::Map(QWidget* parent)
    : QWidget(parent)
    , mapUi(new Ui::Map)
{
    QVariantMap parameters;
    mapUi->setupUi(this);
    mapUi->mapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QQmlEngine* engine = new QQmlEngine(this);
    QQmlComponent* component = new QQmlComponent(engine, "qrc:/qml/CustomMap.qml");
    mapComponent = component->create();
    mapUi->mapWidget->setContent(component->url(), component, mapComponent);
    mapUi->mapWidget->show();
}

Map::~Map()
{
    delete mapComponent;
    delete mapUi;
}

void Map::onGeodeticPosReceived(const GnssPosStruct& pos)
{
    if (mapComponent == nullptr) {
        return;
    }
    double pos_accuracy { std::sqrt(pos.hAcc * pos.hAcc + pos.vAcc * pos.vAcc) };
    QMetaObject::invokeMethod(mapComponent, "setCircle",
        Q_ARG(QVariant, ((double)pos.lon) * 1e-7),
        Q_ARG(QVariant, ((double)pos.lat) * 1e-7),
        Q_ARG(QVariant, pos_accuracy * 1e-3));
}

void Map::onUiEnabledStateChange(bool connected)
{
    if (mapComponent == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(mapComponent, "setEnabled",
        Q_ARG(QVariant, connected));
    mapUi->mapWidget->setEnabled(connected);
}
