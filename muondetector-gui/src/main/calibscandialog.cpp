#include "calibscandialog.h"
#include "ui_calibscandialog.h"
#include <muondetector_structs.h>

CalibScanDialog::CalibScanDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibScanDialog)
{
    ui->setupUi(this);
}

CalibScanDialog::~CalibScanDialog()
{
    delete ui;
}

void CalibScanDialog::onCalibReceived(bool valid, bool eepromValid, quint64 id, const QVector<CalibStruct> &calibList)
{
    if (!eepromValid) return;

    fCalibList.clear();
    for (int i=0; i<calibList.size(); i++)
    {
        fCalibList.push_back(calibList[i]);
    }

    int ver = getCalibParameter("VERSION").toInt();
    double rsense = 0.1*getCalibParameter("RSENSE").toInt();
    double vdiv = 0.01*getCalibParameter("VDIV").toInt();
    int featureFlags = getCalibParameter("FEATURE_FLAGS").toInt();

        fSlope1 = getCalibParameter("COEFF1").toDouble();
        fOffs1 = getCalibParameter("COEFF0").toDouble();
        fSlope2 = getCalibParameter("COEFF3").toDouble();
        fOffs2 = getCalibParameter("COEFF2").toDouble();
}

void CalibScanDialog::onAdcSampleReceived(uint8_t channel, float value)
{
    //
}


void CalibScanDialog::setCalibParameter(const QString &name, const QString &value)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            result->value=value.toStdString();
        }
    }
}

QString CalibScanDialog::getCalibParameter(const QString &name)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            return QString::fromStdString(result->value);
        }
    }
    return "";
}
