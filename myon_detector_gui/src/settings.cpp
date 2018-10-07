#include <settings.h>
#include <ui_settings.h>
#include <ubx_msg_key_name_map.h>

Settings::Settings(QWidget *parent) : QDialog(parent),
settingsUi(new Ui::Settings)
{
    settingsUi->setupUi(this);
    settingsUi->ubloxSignalStates->setColumnCount(2);
    settingsUi->ubloxSignalStates->verticalHeader()->setVisible(false);
    settingsUi->ubloxSignalStates->setShowGrid(false);
    settingsUi->ubloxSignalStates->setAlternatingRowColors(true);
    settingsUi->ubloxSignalStates->setHorizontalHeaderLabels(QStringList({"Signature","Update Rate"}));
    settingsUi->ubloxSignalStates->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}
void Settings::onItemChanged(QTableWidgetItem *item){
    if (item->column()==0){
        return;
    }
    bool ok1 = false;
    bool ok2 = false;
    if (item->text().toInt(&ok1)!=((UbxMsgRateTableItem*)item)->name.toInt(&ok2)){
        if (!ok1 || !ok2){
            if (item->text()==((UbxMsgRateTableItem*)item)->name){
                return;
            }
        }
        settingsUi->ubloxSignalStates->item(item->row(),0)->setCheckState(Qt::Unchecked);
    }
}
void Settings::addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates) {
    oldSettings = ubxMsgRates;
	for (QMap<uint16_t, int>::iterator it = ubxMsgRates.begin(); it != ubxMsgRates.end(); it++) {
        UbxMsgRateTableItem *item = new UbxMsgRateTableItem();
        UbxMsgRateTableItem *value = new UbxMsgRateTableItem();
        item->setCheckState(Qt::CheckState::Checked);
        item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        item->setFlags(item->flags() & (~Qt::ItemIsEditable));
        item->key = it.key();
        item->name = ubxMsgKeyNameMap.value(it.key());
        item->setText(item->name);
        item->setSizeHint(QSize(120,24));
        value->key = it.key();
        value->rate = it.value();
        value->setText(QString::number(value->rate).rightJustified(3, '0'));
        //item->setText(QString(item->name));
        settingsUi->ubloxSignalStates->insertRow(settingsUi->ubloxSignalStates->rowCount());
        settingsUi->ubloxSignalStates->setItem(settingsUi->ubloxSignalStates->rowCount()-1,0,item);
        settingsUi->ubloxSignalStates->setItem(settingsUi->ubloxSignalStates->rowCount()-1,1,value);
	}
    connect(settingsUi->ubloxSignalStates, &QTableWidget::itemChanged, this, &Settings::onItemChanged);
    settingsUi->ubloxSignalStates->resizeColumnsToContents();
    settingsUi->ubloxSignalStates->resizeRowsToContents();
    settingsUi->ubloxSignalStates->setColumnWidth(0,143);
    settingsUi->ubloxSignalStates->setColumnWidth(1,100);
}

void Settings::on_buttonBox_accepted()
{
    QMap<uint16_t, int> changedSettings;
    for (int i=0; i < settingsUi->ubloxSignalStates->rowCount(); i++){
        UbxMsgRateTableItem *item = (UbxMsgRateTableItem*)settingsUi->ubloxSignalStates->item(i,0);
        if (item->checkState()==Qt::Checked){
            continue;
        }
        UbxMsgRateTableItem *value = (UbxMsgRateTableItem*)settingsUi->ubloxSignalStates->item(i,1);
        bool ok = false;
        int newRate = value->text().toInt(&ok);
        if (ok && value->rate != newRate){
             changedSettings.insert(value->key, newRate);
        }
    }
    emit sendSetUbxMsgRateChanges(changedSettings);
}
