#include "mainwindow.h"

/*Q_DECLARE_METATYPE(QVector<QVariant>);
Q_DECLARE_METATYPE(QVector<QString>);
Q_DECLARE_METATYPE(QVector<QIcon>);
*/
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qRegisterMetaTypeStreamOperators<QVector<QVariant<QIcon> > >("QVector<QVariant<QIcon> >");
    //qRegisterMetaTypeStreamOperators<QVector<QVariant(QString)> >("QVector<QVariant(QString)>");
    MainWindow w;
    w.show();
    return a.exec();
}
