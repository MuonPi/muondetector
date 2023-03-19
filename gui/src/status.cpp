#include <QTime>
#include <QtGlobal>
#include <plotcustom.h>
#include <status.h>
#include <ui_status.h>

static const int MAX_BINS = 200; // bins in pulse height histogram
static const float MAX_ADC_VOLTAGE = 4.0;

Status::Status(QWidget* parent)
    : QWidget(parent)
    , statusUi(new Ui::Status)
{
    statusUi->setupUi(this);

    statusUi->pulseHeightHistogram->title = "Pulse Height";

    statusUi->pulseHeightHistogram->setXMin(0.0);
    statusUi->pulseHeightHistogram->setXMax(MAX_ADC_VOLTAGE);
    statusUi->pulseHeightHistogram->setNrBins(MAX_BINS);
    statusUi->pulseHeightHistogram->setLogY(false);
    statusUi->pulseHeightHistogram->setEnabled(false);
    statusUi->pulseHeightHistogramGroupBox->setChecked(false);

    statusUi->ratePlotPresetComboBox->addItems(QStringList() << "seconds"
                                                             << "hh:mm:ss"
                                                             << "time");
    statusUi->ratePlotBufferEdit->setText(QString("%1:%2:%3:%4")
                                              .arg(rateSecondsBuffered / (3600 * 24), 2, 10, QChar('0'))
                                              .arg((rateSecondsBuffered % (3600 * 24)) / 3600, 2, 10, QChar('0'))
                                              .arg((rateSecondsBuffered % (3600)) / 60, 2, 10, QChar('0'))
                                              .arg(rateSecondsBuffered % 60, 2, 10, QChar('0')));

    connect(statusUi->ratePlotBufferEdit, &QLineEdit::editingFinished, this, [this]() { this->setRateSecondsBuffered(statusUi->ratePlotBufferEdit->text()); });
    connect(statusUi->ratePlotPresetComboBox, &QComboBox::currentTextChanged, statusUi->ratePlot, &PlotCustom::setPreset);
    connect(statusUi->resetHistoPushButton, &QPushButton::clicked, this, &Status::clearPulseHeightHisto);
    connect(statusUi->resetRatePushButton, &QPushButton::clicked, this, &Status::clearRatePlot);
    connect(statusUi->resetRatePushButton, &QPushButton::clicked, this, [this]() { this->resetRateClicked(); });
    connect(statusUi->ratePlotGroupBox, &QGroupBox::toggled, statusUi->ratePlot, &PlotCustom::setStatusEnabled);
    connect(statusUi->pulseHeightHistogramGroupBox, &QGroupBox::toggled, statusUi->pulseHeightHistogram, &CustomHistogram::setEnabled);
    connect(statusUi->biasEnableCheckBox, &QCheckBox::clicked, this, &Status::biasSwitchChanged);
    connect(statusUi->highGainCheckBox, &QCheckBox::clicked, this, &Status::gainSwitchChanged);
    connect(statusUi->preamp1CheckBox, &QCheckBox::clicked, this, &Status::preamp1SwitchChanged);
    connect(statusUi->preamp2CheckBox, &QCheckBox::clicked, this, &Status::preamp2SwitchChanged);

    for (const auto& [mux_signal, mux_name] : TIMING_MUX_SIGNAL_NAMES) {
        if (mux_signal != TIMING_MUX_SELECTION::UNDEFINED) {
            statusUi->timingSelectionComboBox->addItem(QString::fromStdString(mux_name));
        }
    }

    for (const auto& [pin, descriptor] : GPIO_SIGNAL_MAP) {
        if (descriptor.direction == DIR_IN) {
            statusUi->triggerSelectionComboBox->addItem(QString::fromStdString(descriptor.name));
        }
    }

    timepulseTimer.setSingleShot(true);
    timepulseTimer.setInterval(3000);
    connect(&timepulseTimer, &QTimer::timeout, this, [this]() {
        statusUi->timePulseLabel->setStyleSheet("QLabel {background-color: red;}");
    });
}

void Status::onGpioRatesReceived(quint8 whichrate, QVector<QPointF> rates)
{
    if (rates.isEmpty())
        return;
    if (whichrate == 0) {
        if (xorSamples.size() > 0) {
            for (int k = rates.size() - 1; k >= 0; k--) {
                bool foundK { false }; // if foundK it has found an index k where the x values of the input points
                // are smaller or equal to the x values of the already existing points -> overlap
                for (int i = xorSamples.size() - 1; i >= xorSamples.size() - 1 - rates.size(); i--) {
                    if (i < 0) {
                        break;
                    }
                    if (rates.at(k).x() <= xorSamples.at(i).x()) {
                        foundK = true;
                    }
                }
                if (foundK && k < rates.size()) {
                    // if there is an overlap, remove all points from input rate points that are already existing
                    rates.remove(0, k + 1);
                    break;
                }
            }
        }

        for (auto rate : rates) {
            xorSamples.append(rate);
        }
        while (!xorSamples.isEmpty() && !rates.isEmpty()) {
            if (xorSamples.first().x() < (rates.last().x() - rateSecondsBuffered)) {
                xorSamples.pop_front();
            } else {
                break;
            }
        }
        statusUi->ratePlot->plotXorSamples(xorSamples);
    }
    if (whichrate == 1) {
        if (andSamples.size() > 0) {
            for (int k = rates.size() - 1; k >= 0; k--) {
                bool foundK = false; // if foundK it has found an index k where the x values of the input points
                // are smaller or equal to the x values of the already existing points -> overlap
                for (int i = andSamples.size() - 1; i >= andSamples.size() - 1 - rates.size(); i--) {
                    if (i < 0) {
                        break;
                    }
                    if (rates.at(k).x() <= andSamples.at(i).x()) {
                        foundK = true;
                    }
                }
                if (foundK && k < rates.size()) {
                    // if there is an overlap, remove all points from input rate points that are already existing
                    rates.remove(0, k + 1);
                    break;
                }
            }
        }

        for (auto rate : rates) {
            andSamples.append(rate);
        }

        while (!andSamples.isEmpty() && !rates.isEmpty()) {
            if (andSamples.first().x() < (rates.last().x() - rateSecondsBuffered)) {
                andSamples.pop_front();
            } else {
                break;
            }
        }
        statusUi->ratePlot->plotAndSamples(andSamples);
    }
}

void Status::setRateSecondsBuffered(const QString& bufferTime)
{
    QStringList values { bufferTime.split(':') };
    quint64 secs { 0 };

    if (values.size() <= 4) {
        for (int i = values.size() - 1; i >= 0; i--) {
            if (i == values.size() - 1) {
                secs += values.at(i).toInt();
            }
            if (i == values.size() - 2) {
                secs += 60 * values.at(i).toInt();
            }
            if (i == values.size() - 3) {
                secs += 3600 * values.at(i).toInt();
            }
            if (i == values.size() - 4) {
                secs += 3600 * 24 * values.at(i).toInt();
            }
        }
        rateSecondsBuffered = secs;
    }
    statusUi->ratePlotBufferEdit->setText(QString("%1:%2:%3:%4")
                                              .arg(rateSecondsBuffered / (3600 * 24), 2, 10, QChar('0'))
                                              .arg((rateSecondsBuffered % (3600 * 24)) / 3600, 2, 10, QChar('0'))
                                              .arg((rateSecondsBuffered % (3600)) / 60, 2, 10, QChar('0'))
                                              .arg(rateSecondsBuffered % 60, 2, 10, QChar('0')));
}

void Status::clearRatePlot()
{
    andSamples.clear();
    xorSamples.clear();
    statusUi->ratePlot->plotXorSamples(xorSamples);
    statusUi->ratePlot->plotAndSamples(andSamples);
}

void Status::onTriggerSelectionReceived(GPIO_SIGNAL signal)
{
    if (!statusUi->triggerSelectionComboBox->isEnabled())
        statusUi->triggerSelectionComboBox->setEnabled(true);
    int i { 0 };
    while (i < statusUi->triggerSelectionComboBox->count()) {
        if (statusUi->triggerSelectionComboBox->itemText(i).compare(QString::fromStdString(GPIO_SIGNAL_MAP.at(signal).name)) == 0)
            break;
        i++;
    }
    if (i >= statusUi->triggerSelectionComboBox->count())
        return;
    statusUi->triggerSelectionComboBox->blockSignals(true);
    statusUi->triggerSelectionComboBox->setEnabled(true);
    statusUi->triggerSelectionComboBox->setCurrentIndex(i);
    statusUi->triggerSelectionComboBox->blockSignals(false);
}

void Status::onTimepulseReceived()
{
    statusUi->timePulseLabel->setStyleSheet("QLabel {background-color: darkGreen;}");
    timepulseTimer.start();
}

void Status::clearPulseHeightHisto()
{
    statusUi->pulseHeightHistogram->clear();
    updatePulseHeightHistogram();
}

void Status::onAdcSampleReceived(uint8_t channel, float value)
{
    if (channel == 0) {
        if (!statusUi->ADCLabel1->isEnabled())
            statusUi->ADCLabel1->setEnabled(true);
        statusUi->ADCLabel1->setText("Ch1: " + QString::number(value, 'f', 3) + " V");
        if (statusUi->pulseHeightHistogram->enabled()) {
            statusUi->pulseHeightHistogram->fill(value);
            updatePulseHeightHistogram();
        }

    } else if (channel == 1) {
        if (!statusUi->ADCLabel2->isEnabled())
            statusUi->ADCLabel2->setEnabled(true);
        statusUi->ADCLabel2->setText("Ch2: " + QString::number(value, 'f', 3) + " V");
    } else if (channel == 2) {
        if (!statusUi->ADCLabel3->isEnabled())
            statusUi->ADCLabel3->setEnabled(true);
        statusUi->ADCLabel3->setText("Ch3: " + QString::number(value, 'f', 3) + " V");
    } else if (channel == 3) {
        if (!statusUi->ADCLabel4->isEnabled())
            statusUi->ADCLabel4->setEnabled(true);
        statusUi->ADCLabel4->setText("Ch4: " + QString::number(value, 'f', 3) + " V");
    }
}

void Status::updatePulseHeightHistogram()
{
    statusUi->pulseHeightHistogram->update();
    long int entries = statusUi->pulseHeightHistogram->getEntries();
    statusUi->PulseHeightHistogramEntriesLabel->setText(QString::number(entries) + " entries");
}

void Status::onUiEnabledStateChange(bool connected)
{
    if (connected) {
        if (statusUi->ratePlotGroupBox->isChecked()) {
            statusUi->ratePlot->setStatusEnabled(true);
        }
        if (statusUi->pulseHeightHistogramGroupBox->isChecked()) {
            statusUi->pulseHeightHistogram->setEnabled(true);
        }
        this->setEnabled(true);
        timepulseTimer.start();
    } else {
        andSamples.clear();
        xorSamples.clear();
        statusUi->ratePlot->setStatusEnabled(false);
        statusUi->pulseHeightHistogram->clear();
        statusUi->pulseHeightHistogram->setEnabled(false);
        statusUi->triggerSelectionComboBox->setEnabled(false);
        statusUi->timingSelectionComboBox->setEnabled(false);
        timepulseTimer.stop();
        statusUi->timePulseLabel->setStyleSheet("QLabel {background-color: Window;}");
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: Window;}");
        statusUi->temperatureLabel->setText("Temperature: N/A");
        statusUi->temperatureLabel->setEnabled(false);
        statusUi->ADCLabel1->setText("Ch1: ");
        statusUi->ADCLabel2->setText("Ch2: ");
        statusUi->ADCLabel3->setText("Ch3: ");
        statusUi->ADCLabel4->setText("Ch4: ");
        statusUi->ADCLabel1->setEnabled(false);
        statusUi->ADCLabel2->setEnabled(false);
        statusUi->ADCLabel3->setEnabled(false);
        statusUi->ADCLabel4->setEnabled(false);
        statusUi->biasEnableCheckBox->setChecked(false);
        statusUi->biasEnableCheckBox->setEnabled(false);
        statusUi->highGainCheckBox->setChecked(false);
        statusUi->highGainCheckBox->setEnabled(false);
        statusUi->preamp1CheckBox->setChecked(false);
        statusUi->preamp2CheckBox->setChecked(false);
        statusUi->preamp1CheckBox->setEnabled(false);
        statusUi->preamp2CheckBox->setEnabled(false);
        statusUi->DACLabel1->setText("Ch1: ");
        statusUi->DACLabel2->setText("Ch2: ");
        statusUi->DACLabel3->setText("Ch3: ");
        statusUi->DACLabel4->setText("Ch4: ");
        statusUi->DACLabel1->setEnabled(false);
        statusUi->DACLabel2->setEnabled(false);
        statusUi->DACLabel3->setEnabled(false);
        statusUi->DACLabel4->setEnabled(false);
        statusUi->triggerSelectionComboBox->setEnabled(false);
        this->setDisabled(true);
    }
}

void Status::on_histoLogYCheckBox_clicked()
{
    statusUi->pulseHeightHistogram->setLogY(statusUi->histoLogYCheckBox->isChecked());
}

void Status::onInputSwitchReceived(TIMING_MUX_SELECTION sel)
{
    if (TIMING_MUX_SIGNAL_NAMES.find(sel) == TIMING_MUX_SIGNAL_NAMES.end())
        return;
    int i = 0;
    while (i < statusUi->timingSelectionComboBox->count()) {
        if (statusUi->timingSelectionComboBox->itemText(i).compare(QString::fromStdString(TIMING_MUX_SIGNAL_NAMES.at(sel))) == 0)
            break;
        i++;
    }
    if (i >= statusUi->timingSelectionComboBox->count())
        return;
    statusUi->timingSelectionComboBox->blockSignals(true);
    statusUi->timingSelectionComboBox->setEnabled(true);
    statusUi->timingSelectionComboBox->setCurrentIndex(i);
    statusUi->timingSelectionComboBox->blockSignals(false);
}

void Status::onBiasSwitchReceived(bool state)
{
    if (!statusUi->biasEnableCheckBox->isEnabled())
        statusUi->biasEnableCheckBox->setEnabled(true);
    statusUi->biasEnableCheckBox->setChecked(state);
}

void Status::onGainSwitchReceived(bool state)
{
    if (!statusUi->highGainCheckBox->isEnabled())
        statusUi->highGainCheckBox->setEnabled(true);
    statusUi->highGainCheckBox->setChecked(state);
}

void Status::onPreampSwitchReceived(uint8_t channel, bool state)
{
    if (channel == 0) {
        if (!statusUi->preamp1CheckBox->isEnabled())
            statusUi->preamp1CheckBox->setEnabled(true);
        statusUi->preamp1CheckBox->setChecked(state);
    } else if (channel == 1) {
        if (!statusUi->preamp2CheckBox->isEnabled())
            statusUi->preamp2CheckBox->setEnabled(true);
        statusUi->preamp2CheckBox->setChecked(state);
    }
}

void Status::onDacReadbackReceived(uint8_t channel, float value)
{
    if (channel == 0) {
        if (!statusUi->DACLabel1->isEnabled())
            statusUi->DACLabel1->setEnabled(true);
        statusUi->DACLabel1->setText("Ch1: " + QString::number(value, 'f', 3) + " V");
    } else if (channel == 1) {
        if (!statusUi->DACLabel2->isEnabled())
            statusUi->DACLabel2->setEnabled(true);
        statusUi->DACLabel2->setText("Ch2: " + QString::number(value, 'f', 3) + " V");
    } else if (channel == 2) {
        if (!statusUi->DACLabel3->isEnabled())
            statusUi->DACLabel3->setEnabled(true);
        statusUi->DACLabel3->setText("Ch3: " + QString::number(value, 'f', 3) + " V");
    } else if (channel == 3) {
        if (!statusUi->DACLabel4->isEnabled())
            statusUi->DACLabel4->setEnabled(true);
        statusUi->DACLabel4->setText("Ch4: " + QString::number(value, 'f', 3) + " V");
    }
}

void Status::onTemperatureReceived(float value)
{
    if (!statusUi->temperatureLabel->isEnabled())
        statusUi->temperatureLabel->setEnabled(true);
    statusUi->temperatureLabel->setText("Temperature: " + QString::number(value, 'f', 2) + " °C");
}

Status::~Status()
{
    delete statusUi;
}

void Status::on_triggerSelectionComboBox_currentIndexChanged(const QString& arg1)
{
    for (const auto& [pin, descriptor] : GPIO_SIGNAL_MAP) {
        if (QString::fromStdString(descriptor.name) == arg1) {
            emit triggerSelectionChanged(pin);
            return;
        }
    }
}

void Status::on_timingSelectionComboBox_currentIndexChanged(const QString& arg)
{
    for (const auto& [mux_signal, mux_name] : TIMING_MUX_SIGNAL_NAMES) {
        if (QString::fromStdString(mux_name) == arg) {
            emit inputSwitchChanged(mux_signal);
            return;
        }
    }
}

void Status::onMqttStatusChanged(bool connected)
{
    if (connected) {
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: darkGreen;}");
        statusUi->mqttStatusLabel->setText("MQTT: Connected");
    } else {
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: red;}");
        statusUi->mqttStatusLabel->setText("MQTT: No Connection");
    }
}

void Status::onMqttStatusChanged(MuonPi::MqttHandler::Status status)
{
    QString status_string { "N/A" };
    switch (status) {
    case MuonPi::MqttHandler::Status::Connected:
        status_string = "Connected";
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: darkGreen;}");
        break;
    case MuonPi::MqttHandler::Status::Disconnecting:
    case MuonPi::MqttHandler::Status::Connecting:
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: yellow;}");
        status_string = "Connecting/Disconnecting";
        break;
    case MuonPi::MqttHandler::Status::Disconnected:
        status_string = "Disconnected";
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: window;}");
        break;
    case MuonPi::MqttHandler::Status::Inhibited:
        status_string = "Inhibited";
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: yellow;}");
        break;
    case MuonPi::MqttHandler::Status::Error:
    default:
        status_string = "Error";
        statusUi->mqttStatusLabel->setStyleSheet("QLabel {background-color: red;}");
    };
    statusUi->mqttStatusLabel->setText("MQTT: " + status_string);
}
