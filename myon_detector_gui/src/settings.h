#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QListWidget>

class UbxMsgRateListItem : public QListWidgetItem
{
public:
    using QListWidgetItem::QListWidgetItem;
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

public slots:
    void addUbxMsgRates(QMap<uint16_t,int> ubxMsgRates);
private:
    Ui::Settings *settingsUi;
};

#endif // SETTINGS_H
