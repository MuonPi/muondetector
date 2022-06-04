#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSsl>
#include <cmath>
#include <map.h>
#include <muondetector_structs.h>

#include <ui_map.h>

Map::Map(QWidget* parent)
    : QWidget(parent)
    , mapUi(new Ui::Map)
{
    QVariantMap parameters;
    mapUi->setupUi(this);

    for ( const auto& item: PositionModeConfig::name ) {
        mapUi->modeComboBox->addItem(QString::fromLocal8Bit(item));
    }

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

void Map::onPosConfigReceived(const PositionModeConfig &pos)
{
    mapUi->modeComboBox->setCurrentIndex(pos.mode);
    mapUi->longitudeLineEdit->setText(QString::number(pos.static_position.longitude));
    mapUi->latitudeLineEdit->setText(QString::number(pos.static_position.latitude));
    mapUi->altitudeLineEdit->setText(QString::number(pos.static_position.altitude));
    mapUi->horErrorLineEdit->setText(QString::number(pos.static_position.hor_error));
    mapUi->vertErrorLineEdit->setText(QString::number(pos.static_position.vert_error));
}

void Map::onUiEnabledStateChange(bool connected)
{
    if (mapComponent == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(mapComponent, "setEnabled",
        Q_ARG(QVariant, connected));
    mapUi->mapWidget->setEnabled(connected);
    mapUi->modeComboBox->setEnabled(connected);
}

void Map::on_setConfigPushButton_clicked()
{
    PositionModeConfig posconfig {};
    posconfig.mode = static_cast<PositionModeConfig::Mode>(mapUi->modeComboBox->currentIndex());
    bool ok { false };
    posconfig.static_position.longitude = mapUi->longitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.latitude = mapUi->latitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.altitude = mapUi->altitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.hor_error = mapUi->horErrorLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.vert_error = mapUi->vertErrorLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    emit posModeConfigChanged(posconfig);
}
