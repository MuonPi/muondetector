#include <map.h>
#include <ui_map.h>
#include <QQmlEngine>
#include <QQmlComponent>

Map::Map(QWidget *parent) :
    QWidget(parent),
    mapUi(new Ui::Map)
{
    mapUi->setupUi(this);
    mapUi->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QQmlEngine* engine = new QQmlEngine(this);
#if defined(Q_OS_UNIX)
        QQmlComponent* component = new QQmlComponent(engine, ":/qml/mymap.qml");//QUrl::fromLocalFile("/usr/share/muondetector-gui/qml/mymap.qml"));
#elif defined(Q_OS_WIN)
        QQmlComponent* component = new QQmlComponent(engine, QUrl::fromLocalFile("qml/mymap.qml"));
#else
        // put error message here
#endif
    mapComponent = component->create();
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

