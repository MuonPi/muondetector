#ifndef SETTINGS_H
#define SETTINGS_H

#include "ublox/ublox_structs.h"

#include <QDialog>
#include <QTableWidget>
#include <cstdint>
#include <events/ubx_event.h>
#include <qabstractbutton.h>
#include <qmap.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwidget.h>
#include <vector>

class UbxMsgRateTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    uint16_t key;
    int rate;
    QString name;
};

namespace Ui {
class UbloxSettingsForm;
}

class UbloxSettingsForm : public QDialog {
    Q_OBJECT

  public:
    explicit UbloxSettingsForm(QWidget* parent = nullptr);

  signals:
    void sendSetCfgMsgRateChange(uint16_t msgID, int rate);
    void sendRequestCfgMsgRates();
    void sendUbxReset();
    void sendUbxConfigDefault();
    void setGnssConfigs(const std::vector<GnssConfigStruct>& configList);
    void setTP5Config(const UbxTimePulseStruct& tp);
    void sendUbxSaveCfg();

  public slots:
    void addCfgMsgRate(const CfgMsg& rate);
    void onItemChanged(QTableWidgetItem* item);
    void onUiEnabledStateChange(bool connected);
    void onTxBufReceived(quint8 val);
    void onTxBufPeakReceived(quint8 val);
    void onRxBufReceived(quint8 val);
    void onRxBufPeakReceived(quint8 val);
    void onGnssConfigsReceived(quint8 numTrkCh, const std::vector<GnssConfigStruct>& configList);
    void onTP5Received(const UbxTimePulseStruct& tp);
    void onConfigChanged();

  private slots:
    void onSettingsButtonBoxClicked(QAbstractButton* button);
    void onUbxResetPushButtonClicked();
    void writeGnssConfig();
    void writeTpConfig();

    void onTimeGridComboBoxCurrentIndexChanged(int index);

    void onFreqPeriodLineEditEditingFinished();

    void onFreqPeriodLockLineEditEditingFinished();

    void onPulseLenLineEditEditingFinished();

    void onPulseLenLockLineEditEditingFinished();

    void onAntDelayLineEditEditingFinished();

    void onGroupDelayLineEditEditingFinished();

    void onUserDelayLineEditEditingFinished();

    void onSaveConfigPushButtonClicked();

  private:
    Ui::UbloxSettingsForm* ui;
    QMap<uint16_t, int> oldSettings;
    bool fGnssConfigChanged = false;
    bool fTpConfigChanged = false;
    UbxTimePulseStruct fTpConfig;
};

#endif // SETTINGS_H
