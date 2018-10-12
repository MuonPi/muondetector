#include "map.h"
#include "ui_map.h"

Map::Map(QWidget *parent) :
    QWidget(parent),
    mapUi(new Ui::Map)
{
    mapUi->setupUi(this);
    mapUi->quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mapUi->quickWidget->setSource(QUrl::fromLocalFile("src/mymap.qml"));
}

Map::~Map()
{
    delete mapUi;
}
