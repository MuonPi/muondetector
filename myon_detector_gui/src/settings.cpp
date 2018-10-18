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
    connect(settingsUi->settingsButtonBox, &QDialogButtonBox::clicked, this, &Settings::onSettingsButtonBoxClicked);
    connect(settingsUi->ubloxSignalStates, &QTableWidget::itemChanged, this, &Settings::onItemChanged);
    settingsUi->ubloxSignalStates->blockSignals(true);
    this->setDisabled(true);
    emit sendRequestUbxMsgRates();
}

void Settings::onItemChanged(QTableWidgetItem *item){
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setEnabled(true);
    if (item->column()==0){
        return;
    }
    bool ok = false;
    if (item->text().toInt(&ok)!=((UbxMsgRateTableItem*)item)->rate){
        if(!ok){
            qDebug() << "rate is no integer";
        }
        settingsUi->ubloxSignalStates->item(item->row(),0)->setCheckState(Qt::Unchecked);
    }else{
        settingsUi->ubloxSignalStates->item(item->row(),0)->setCheckState(Qt::Checked);
    }
}

void Settings::addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates) {
    settingsUi->ubloxSignalStates->clearContents();
    settingsUi->ubloxSignalStates->setRowCount(0);
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
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
        // I don't know why item gets (sometimes) uncheckd when transfered to the TableView
        // it seems to work now though:
        item->setCheckState(Qt::CheckState::Checked);
	}
    settingsUi->ubloxSignalStates->blockSignals(false);
    settingsUi->ubloxSignalStates->resizeColumnsToContents();
    settingsUi->ubloxSignalStates->resizeRowsToContents();
    settingsUi->ubloxSignalStates->setColumnWidth(0,143);
    settingsUi->ubloxSignalStates->setColumnWidth(1,100);
}

void Settings::onSettingsButtonBoxClicked(QAbstractButton *button){
    if (button == settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)){
        settingsUi->ubloxSignalStates->blockSignals(true);
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
        if (changedSettings.size()>0){
            emit sendSetUbxMsgRateChanges(changedSettings);
        }else{
            settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
            settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
        }
    }
    if (button == settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)){
        settingsUi->ubloxSignalStates->blockSignals(true);
        emit sendRequestUbxMsgRates();
    }
    if (button == settingsUi->settingsButtonBox->button(QDialogButtonBox::RestoreDefaults)){
        //settingsUi->ubloxSignalStates->blockSignals(true);

    }
}

void Settings::onUiEnabledStateChange(bool connected){
    if (connected){
        this->setEnabled(true);
        //settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
        //settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
    }else{
        settingsUi->ubloxSignalStates->clearContents();
        settingsUi->ubloxSignalStates->setRowCount(0);
        settingsUi->ubloxSignalStates->blockSignals(true);
        this->setDisabled(true);
    }
}

