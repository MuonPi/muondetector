#include <mainwindow.h>
#include <QApplication>

int main(int argc, char *argv[])
{
    //qRegisterMetaType<uint16_t>("uint16_t");
    //qRegisterMetaType<QMap<uint16_t, int> >("QMap<uint16_t,int>");
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
