#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QTableWidget>
#include <QtWidgets>
#include <geodeticpos.h>

class UbxMsgRateTableItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;
	uint16_t key;
	int rate;
	QString name;
};

namespace Ui {
	class Settings;
}

class Settings : public QDialog
{
	Q_OBJECT
public:
	explicit Settings(QWidget *parent = nullptr);

signals:
    void sendSetUbxMsgRateChanges(QMap<uint16_t, int> ubxMsgRateChanges);
    void sendRequestUbxMsgRates();

public slots:
	void addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates);
    void onItemChanged(QTableWidgetItem *item);
    void onGeodeticPosReceived(GeodeticPos pos);
    void onUiEnabledStateChange(bool connected);

private slots:

    void onSettingsButtonBoxClicked(QAbstractButton *button);

private:
    Ui::Settings *settingsUi;
    QMap<uint16_t, int> oldSettings;
};

#endif // SETTINGS_H
