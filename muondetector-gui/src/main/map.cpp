#include <map.h>
#include <ui_map.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QSsl>

Map::Map(QWidget *parent) :
    QWidget(parent),
    mapUi(new Ui::Map)
{
    QVariantMap parameters;
    mapUi->setupUi(this);
    mapUi->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Fetch tokens from the environment, if present
    const QByteArray mapboxMapID = qgetenv("MAPBOX_MAP_ID");
    const QByteArray mapboxAccessToken = qgetenv("MAPBOX_ACCESS_TOKEN");
    const QByteArray hereAppID = qgetenv("HERE_APP_ID");
    const QByteArray hereToken = qgetenv("HERE_TOKEN");
    const QByteArray esriToken = qgetenv("ESRI_TOKEN");
    if (!mapboxMapID.isEmpty()){
        parameters["mapbox.map_id"] = QString::fromLocal8Bit(mapboxMapID);
    }
    if (!mapboxAccessToken.isEmpty()){
        parameters["mapbox.access_token"] = QString::fromLocal8Bit(mapboxAccessToken);
    }
    if (!hereAppID.isEmpty()){
        parameters["here.app_id"] = QString::fromLocal8Bit(hereAppID);
    }
    if (!hereToken.isEmpty()){
        parameters["here.token"] = QString::fromLocal8Bit(hereToken);
    }
    if (!esriToken.isEmpty()){
        parameters["esri.token"] = QString::fromLocal8Bit(esriToken);
    }

    QQmlEngine* engine = new QQmlEngine(this);
    engine->addImportPath(QStringLiteral(":/imports"));
    QQmlComponent* component = new QQmlComponent(engine, "qrc:/qml/mymap.qml");//QUrl::fromLocalFile("/usr/share/muondetector-gui/qml/mymap.qml"));
    mapComponent = component->create();
    QMetaObject::invokeMethod(mapComponent, "initializeProviders",
                              Q_ARG(QVariant, QVariant::fromValue(parameters)));
    mapUi->quickWidget->setContent(component->url(), component, mapComponent);
    mapUi->quickWidget->show();
    //mapUi->quickWidget->setSource(QUrl::fromLocalFile("qml/mymap.qml"));
}

Map::~Map()
{
    delete mapComponent;
    delete mapUi;
}

void Map::onGeodeticPosReceived(GeodeticPos pos){
    //qDebug() << (double)pos.lon*1e-7 << (double)pos.lat*1e-7 << (double)pos.hAcc/1000.0;
    if (mapComponent==nullptr){
        return;
    }
    QMetaObject::invokeMethod(mapComponent, "setCircle",
                              Q_ARG(QVariant, ((double)pos.lon)*1e-7),
                              Q_ARG(QVariant, ((double)pos.lat)*1e-7),
                              Q_ARG(QVariant, ((double)pos.hAcc)/1000));
}

