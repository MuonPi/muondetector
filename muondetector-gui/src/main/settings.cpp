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
    connect(settingsUi->gnssConfigButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int)
    {
        this->fGnssConfigChanged=true;
        this->settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        this->settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setEnabled(true);
    } );
    connect(settingsUi->tpConfigButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [this](int)
    {
        this->fTpConfigChanged=true;
        this->settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        this->settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setEnabled(true);
    } );
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
    settingsUi->ubloxSignalStates->blockSignals(true);
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
        } else if (fGnssConfigChanged) {
            fGnssConfigChanged=false;
            writeGnssConfig();
        } else if (fTpConfigChanged) {
            fTpConfigChanged=false;
            writeTpConfig();
        }
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
        settingsUi->ubloxSignalStates->blockSignals(false);
    }
    if (button == settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)){
        onTP5Received(fTpConfig);
        settingsUi->ubloxSignalStates->blockSignals(true);
        emit sendRequestUbxMsgRates();
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
    }
    if (button == settingsUi->settingsButtonBox->button(QDialogButtonBox::RestoreDefaults)){
        //settingsUi->ubloxSignalStates->blockSignals(true);
        emit sendUbxConfigDefault();

    }
}

void Settings::onUiEnabledStateChange(bool connected){
    if (connected){
        this->setEnabled(true);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
        settingsUi->ubloxSignalStates->blockSignals(false);
    } else {
        settingsUi->ubloxSignalStates->clearContents();
        settingsUi->ubloxSignalStates->setRowCount(0);
        settingsUi->ubloxSignalStates->blockSignals(true);
        settingsUi->gnssConfigButtonGroup->blockSignals(true);
        settingsUi->gnssGpsCheckBox->setEnabled(false);
        settingsUi->gnssGpsCheckBox->setChecked(false);
        settingsUi->gnssSbasCheckBox->setEnabled(false);
        settingsUi->gnssSbasCheckBox->setChecked(false);
        settingsUi->gnssGalCheckBox->setEnabled(false);
        settingsUi->gnssGalCheckBox->setChecked(false);
        settingsUi->gnssBeidCheckBox->setEnabled(false);
        settingsUi->gnssBeidCheckBox->setChecked(false);
        settingsUi->gnssQzssCheckBox->setEnabled(false);
        settingsUi->gnssQzssCheckBox->setChecked(false);
        settingsUi->gnssGlnsCheckBox->setEnabled(false);
        settingsUi->gnssGlnsCheckBox->setChecked(false);
        settingsUi->gnssImesCheckBox->setEnabled(false);
        settingsUi->gnssImesCheckBox->setChecked(false);
        settingsUi->numTrkChannelsLabel->setText("N/A");
        settingsUi->gnssConfigButtonGroup->blockSignals(true);
        this->setDisabled(true);
    }
}

void Settings::onTxBufReceived(quint8 val)
{
    settingsUi->txBufProgressBar->setValue(val);
}

void Settings::onTxBufPeakReceived(quint8 val)
{
    settingsUi->txPeakLabel->setText("max: "+QString::number(val)+"%");
}

//const QString GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
void Settings::onGnssConfigsReceived(quint8 numTrkCh, const QVector<GnssConfigStruct> &configList)
{
    settingsUi->gnssConfigButtonGroup->blockSignals(true);
    settingsUi->numTrkChannelsLabel->setText(QString::number(numTrkCh));
    for (int i=0; i<configList.size(); i++) {
        if (configList[i].gnssId==0) { // GPS
            settingsUi->gnssGpsCheckBox->setEnabled(true);
            settingsUi->gnssGpsCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==1) { // SBAS
            settingsUi->gnssSbasCheckBox->setEnabled(true);
            settingsUi->gnssSbasCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==2) { // GAL
            settingsUi->gnssGalCheckBox->setEnabled(true);
            settingsUi->gnssGalCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==3) { // BEID
            settingsUi->gnssBeidCheckBox->setEnabled(true);
            settingsUi->gnssBeidCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==4) { // IMES
            settingsUi->gnssImesCheckBox->setEnabled(true);
            settingsUi->gnssImesCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==5) { // QZSS
            settingsUi->gnssQzssCheckBox->setEnabled(true);
            settingsUi->gnssQzssCheckBox->setChecked(configList[i].flags & 0x01);
        } else if (configList[i].gnssId==6) { // GLNS
            settingsUi->gnssGlnsCheckBox->setEnabled(true);
            settingsUi->gnssGlnsCheckBox->setChecked(configList[i].flags & 0x01);
        }
    }
    settingsUi->gnssConfigButtonGroup->blockSignals(false);
    fGnssConfigChanged = false;
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(true);
}

void Settings::onTP5Received(const UbxTimePulseStruct &tp)
{
    fTpConfig=tp;
    settingsUi->tpConfigButtonGroup->blockSignals(true);
    settingsUi->antDelayLineEdit->setText(QString::number(tp.antCableDelay));
    settingsUi->groupDelayLineEdit->setText(QString::number(tp.rfGroupDelay));
    settingsUi->userDelayLineEdit->setText(QString::number(tp.userConfigDelay));
    settingsUi->freqPeriodLineEdit->setText(QString::number(tp.freqPeriod));
    settingsUi->freqPeriodLockLineEdit->setText(QString::number(tp.freqPeriodLock));
    settingsUi->pulseLenLineEdit->setText(QString::number(tp.pulseLenRatio));
    settingsUi->pulseLenLockLineEdit->setText(QString::number(tp.pulseLenRatioLock));
    settingsUi->tpActiveCheckBox->setChecked(tp.flags & UbxTimePulseStruct::ACTIVE);
    settingsUi->lockGpsCheckBox->setChecked(tp.flags & UbxTimePulseStruct::LOCK_GPS);
    settingsUi->lockOtherCheckBox->setChecked(tp.flags & UbxTimePulseStruct::LOCK_OTHER);
    settingsUi->timeGridComboBox->setCurrentIndex((tp.flags & UbxTimePulseStruct::GRID_UTC_GPS)>>7);
    settingsUi->tpPolarityCheckBox->setChecked((tp.flags & UbxTimePulseStruct::POLARITY));
    settingsUi->tpAlignTowCheckBox->setChecked((tp.flags & UbxTimePulseStruct::ALIGN_TO_TOW));
    settingsUi->tpConfigButtonGroup->blockSignals(false);
}


void Settings::on_ubxResetPushButton_clicked()
{
    // reset Ublox device
    emit sendUbxReset();
}


void Settings::writeGnssConfig()
{
    QVector<GnssConfigStruct> configList;
    if (settingsUi->gnssGpsCheckBox->isEnabled()) { // GPS
       GnssConfigStruct config;
       config.gnssId=0; config.maxTrkCh=14; config.resTrkCh=8;
       config.flags = (settingsUi->gnssGpsCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssSbasCheckBox->isEnabled()) { // SBAS
       GnssConfigStruct config;
       config.gnssId=1; config.maxTrkCh=3; config.resTrkCh=1;
       config.flags = (settingsUi->gnssSbasCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssGalCheckBox->isEnabled()) { // GAL
       GnssConfigStruct config;
       config.gnssId=2; config.maxTrkCh=8; config.resTrkCh=4;
       config.flags = (settingsUi->gnssGalCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssBeidCheckBox->isEnabled()) { // BEID
       GnssConfigStruct config;
       config.gnssId=3; config.maxTrkCh=10; config.resTrkCh=4;
       config.flags = (settingsUi->gnssBeidCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssImesCheckBox->isEnabled()) { // IMES
       GnssConfigStruct config;
       config.gnssId=4; config.maxTrkCh=8; config.resTrkCh=0;
       config.flags = (settingsUi->gnssImesCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssQzssCheckBox->isEnabled()) { // QZSS
       GnssConfigStruct config;
       config.gnssId=5; config.maxTrkCh=3; config.resTrkCh=0;
       config.flags = (settingsUi->gnssQzssCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (settingsUi->gnssGlnsCheckBox->isEnabled()) { // GLNS
       GnssConfigStruct config;
       config.gnssId=6; config.maxTrkCh=12; config.resTrkCh=6;
       config.flags = (settingsUi->gnssGlnsCheckBox->isChecked())?0x00000001:0;
       configList.push_back(config);
    }
    if (configList.size()) emit setGnssConfigs(configList);
}

void Settings::writeTpConfig()
{
    UbxTimePulseStruct tp;
    tp.tpIndex=0;
    bool ok=false;
    tp.antCableDelay=settingsUi->antDelayLineEdit->text().toInt(&ok);
    if (!ok) return;
    tp.rfGroupDelay=settingsUi->groupDelayLineEdit->text().toInt(&ok);
    if (!ok) return;
    tp.userConfigDelay=settingsUi->userDelayLineEdit->text().toLong(&ok);
    if (!ok) return;
    tp.freqPeriod=settingsUi->freqPeriodLineEdit->text().toLong(&ok);
    if (!ok) return;
    tp.freqPeriodLock=settingsUi->freqPeriodLockLineEdit->text().toLong(&ok);
    if (!ok) return;
    tp.pulseLenRatio=settingsUi->pulseLenLineEdit->text().toLong(&ok);
    if (!ok) return;
    tp.pulseLenRatioLock=settingsUi->pulseLenLockLineEdit->text().toLong(&ok);
    if (!ok) return;
    tp.flags = UbxTimePulseStruct::IS_LENGTH;
    if (settingsUi->tpActiveCheckBox->isChecked()) tp.flags |= UbxTimePulseStruct::ACTIVE;
    if (settingsUi->tpPolarityCheckBox->isChecked()) tp.flags |= UbxTimePulseStruct::POLARITY;
    if (settingsUi->lockGpsCheckBox->isChecked()) tp.flags |= UbxTimePulseStruct::LOCK_GPS;
    if (settingsUi->lockOtherCheckBox->isChecked()) tp.flags |= UbxTimePulseStruct::LOCK_OTHER;
    if (settingsUi->tpAlignTowCheckBox->isChecked()) tp.flags |= UbxTimePulseStruct::ALIGN_TO_TOW;
    tp.flags |= ((settingsUi->timeGridComboBox->currentIndex()<<7) & UbxTimePulseStruct::GRID_UTC_GPS);
    emit setTP5Config(tp);
}


void Settings::on_timeGridComboBox_currentIndexChanged(int index)
{
    fTpConfigChanged = true;
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
    settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
}

void Settings::on_freqPeriodLineEdit_editingFinished()
{
    bool ok = false;
    int freqPeriod = settingsUi->freqPeriodLineEdit->text().toLong(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}

void Settings::on_freqPeriodLockLineEdit_editingFinished()
{
    bool ok = false;
    int freqPeriod = settingsUi->freqPeriodLockLineEdit->text().toLong(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}

void Settings::on_pulseLenLineEdit_editingFinished()
{
    bool ok = false;
    int len = settingsUi->pulseLenLineEdit->text().toLong(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}

void Settings::on_pulseLenLockLineEdit_editingFinished()
{
    bool ok = false;
    int len = settingsUi->pulseLenLockLineEdit->text().toLong(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}

void Settings::on_antDelayLineEdit_editingFinished()
{
    bool ok = false;
    int delay = settingsUi->antDelayLineEdit->text().toInt(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }

}

void Settings::on_groupDelayLineEdit_editingFinished()
{
    bool ok = false;
    int delay = settingsUi->groupDelayLineEdit->text().toInt(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}

void Settings::on_userDelayLineEdit_editingFinished()
{
    bool ok = false;
    int delay = settingsUi->userDelayLineEdit->text().toLong(&ok);
    if (!ok) onTP5Received(fTpConfig);
    else {
        fTpConfigChanged = true;
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
        settingsUi->settingsButtonBox->button(QDialogButtonBox::Discard)->setDisabled(false);
    }
}
