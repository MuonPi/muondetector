#include <QThread>
#include "calibform.h"
#include "ui_calibform.h"
#include <qwt_symbol.h>
#include <string>
#include <muondetector_structs.h>

#define calVoltMin 0.3
#define calVoltMax 2.5

#define BIAS_SCAN_INCREMENT 0.025  // scan step in Volts


using namespace std;

const static CalibStruct invalidCalibItem;

inline static double sqr(double x) {
        return x*x;
}

/*
 linear regression algorithm
 taken from:
   http://stackoverflow.com/questions/5083465/fast-efficient-least-squares-fit-algorithm-in-c
 parameters:
  n = number of data points
  xarray,yarray  = arrays of data
  *offs = output intercept
  *slope  = output slope
 */
bool calcLinearCoefficients( const QVector<QPointF>& points,
        /*int n, double *xarray, double *yarray,
                */double *offs, double* slope)
{
   int n=points.size();
   if (n<3) return false;

   double   sumx = 0.0;                        /* sum of x                      */
   double   sumx2 = 0.0;                       /* sum of x**2                   */
   double   sumxy = 0.0;                       /* sum of x * y                  */
   double   sumy = 0.0;                        /* sum of y                      */
   double   sumy2 = 0.0;                       /* sum of y**2                   */

//   int ix=0;
//   double offsx=xarray[ix];
//   double offsy=yarray[ix];
   double offsx=0;
   double offsy=0;
//    long long int offsy=0;

   for (int i=0; i<n; i++) {
          sumx  += points[i].x()-offsx;
          sumx2 += sqr(points[i].x()-offsx);
          sumxy += (points[i].x()-offsx) * (points[i].y()-offsy);
          sumy  += (points[i].y()-offsy);
          sumy2 += sqr(points[i].y()-offsy);
   }


   double denom = (n * sumx2 - sqr(sumx));
   if (denom == 0) {
       // singular matrix. can't solve the problem.
       *slope = 0;
       *offs = 0;
//       if (r) *r = 0;
       return false;
   }

   double m = (n * sumxy  -  sumx * sumy) / denom;
   double b = (sumy * sumx2  -  sumx * sumxy) / denom;

   *slope=m;
   *offs=b+offsy;
   return true;
//    *offs=b;
//   printf("offsI=%lld  offsF=%f\n", offsy, b);

}


CalibForm::CalibForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalibForm)
{
    ui->setupUi(this);
    ui->calibItemTableWidget->resizeColumnsToContents();

    ui->biasVoltageCalibPlot->setTitle("Ubias calibration");
    ui->biasVoltageCalibPlot->setAxisTitle(QwtPlot::xBottom,"DAC voltage / V");
    ui->biasVoltageCalibPlot->setAxisTitle(QwtPlot::yLeft,"Ubias / V");
    ui->biasCurrentCalibPlot->setTitle("Ibias correction");
    ui->biasCurrentCalibPlot->setAxisTitle(QwtPlot::xBottom,"bias voltage / V");
    QFont font = ui->biasCurrentCalibPlot->axisTitle(QwtPlot::yLeft).font();
    font.setPointSize(5);
    ui->biasCurrentCalibPlot->axisTitle(QwtPlot::yLeft).font().setPointSize(5);
    ui->biasCurrentCalibPlot->setAxisTitle(QwtPlot::yLeft,"Ibias / uA");

    ui->biasVoltageCalibPlot->addCurve("curve1", Qt::blue);
    ui->biasVoltageCalibPlot->curve("curve1").setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol *sym=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::blue, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->biasVoltageCalibPlot->curve("curve1").setSymbol(sym);

    ui->biasVoltageCalibPlot->addCurve("curve2", Qt::red);
    ui->biasVoltageCalibPlot->curve("curve2").setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol *sym2=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::red, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->biasVoltageCalibPlot->curve("curve2").setSymbol(sym2);

    ui->biasCurrentCalibPlot->addCurve("curve3", Qt::green);
    ui->biasCurrentCalibPlot->curve("curve3").setStyle(QwtPlotCurve::NoCurve);
    //ui->biasCurrentCalibPlot->curve("curve3").setYAxis(QwtPlot::yRight);
    QwtSymbol *sym3=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::green, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->biasCurrentCalibPlot->curve("curve3").setSymbol(sym3);
    //ui->biasCurrentCalibPlot->enableAxis(QwtPlot::yRight,true);
    ui->biasCurrentCalibPlot->addCurve("Fit", Qt::green);
    ui->biasCurrentCalibPlot->curve("Fit").setStyle(QwtPlotCurve::Lines);

    ui->biasVoltageCalibPlot->addCurve("Fit1", Qt::blue);
    ui->biasVoltageCalibPlot->curve("Fit1").setStyle(QwtPlotCurve::Lines);
    ui->biasVoltageCalibPlot->addCurve("Fit2", Qt::red);
    ui->biasVoltageCalibPlot->curve("Fit2").setStyle(QwtPlotCurve::Lines);

    ui->biasVoltageCalibPlot->replot();
    ui->biasCurrentCalibPlot->replot();
    ui->transferBiasCoeffsPushButton->setEnabled(false);

}

CalibForm::~CalibForm()
{
    delete ui;
}

void CalibForm::onCalibReceived(bool valid, bool eepromValid, quint64 id, const QVector<CalibStruct> & calibList)
{
    QString str = "invalid";
    if (eepromValid) str="valid";
    ui->eepromValidLabel->setText("EEPROM data: "+str);
    str = "invalid";
    if (valid) str="valid";
    ui->calibValidLabel->setText("Calib data: "+str);
    ui->idLineEdit->setText(QString::number(id,16));

    fCalibList.clear();
    for (int i=0; i<calibList.size(); i++)
    {
        fCalibList.push_back(calibList[i]);
    }

    int ver = getCalibParameter("VERSION").toInt();
    ui->eepromHwVersionSpinBox->setValue(ver);
    double rsense = 0.1*getCalibParameter("RSENSE").toInt();
    ui->rsenseDoubleSpinBox->setValue(rsense);
    double vdiv = 0.01*getCalibParameter("VDIV").toInt();
    ui->vdivDoubleSpinBox->setValue(vdiv);
    int eepCycles = getCalibParameter("WRITE_CYCLES").toInt();
    ui->eepromWriteCyclesLabel->setText(QString::number(eepCycles));
    int featureFlags = getCalibParameter("FEATURE_FLAGS").toInt();
    ui->featureGnssCheckBox->setChecked(featureFlags & CalibStruct::FEATUREFLAGS_GNSS);
    ui->featureEnergyCheckBox->setChecked(featureFlags & CalibStruct::FEATUREFLAGS_ENERGY);
    ui->featureDetBiasCheckBox->setChecked(featureFlags & CalibStruct::FEATUREFLAGS_DETBIAS);
    ui->featurePreampBiasCheckBox->setChecked(featureFlags & CalibStruct::FEATUREFLAGS_PREAMP_BIAS);

    if (voltageCalibValid()) {
        fSlope1 = getCalibParameter("COEFF1").toDouble();
        fOffs1 = getCalibParameter("COEFF0").toDouble();
    }
    if (currentCalibValid()) {
        fSlope2 = getCalibParameter("COEFF3").toDouble();
        fOffs2 = getCalibParameter("COEFF2").toDouble();
    }
    updateCalibTable();
    QVector<CalibStruct> emptyList;
    emit updatedCalib(emptyList);
}

void CalibForm::updateCalibTable()
{
    ui->calibItemTableWidget->setRowCount(fCalibList.size());
    for (int i=0; i<fCalibList.size(); i++)
    {
        QTableWidgetItem *newItem1 = new QTableWidgetItem(QString::fromStdString(fCalibList[i].name));
        newItem1->setSizeHint(QSize(90,20));
        ui->calibItemTableWidget->setItem(i, 0, newItem1);
        QString type = QString::fromStdString(fCalibList[i].type);
        QString numberstr = "";
        if (type=="FLOAT") {
            double val = QString::fromStdString(fCalibList[i].value).toDouble(NULL);
            numberstr = QString::number(val);
        } else {
            numberstr = QString::fromStdString(fCalibList[i].value);
        }

        QTableWidgetItem *newItem2 = new QTableWidgetItem(type);
        newItem2->setSizeHint(QSize(40,20));
        ui->calibItemTableWidget->setItem(i, 1, newItem2);
        QTableWidgetItem *newItem3 = new QTableWidgetItem(numberstr);
        newItem3->setSizeHint(QSize(60,20));
        ui->calibItemTableWidget->setItem(i, 2, newItem3);
        QTableWidgetItem *newItem4 = new QTableWidgetItem("0x"+QString("%1").arg(fCalibList[i].address, 2, 16, QChar('0')));
        newItem4->setSizeHint(QSize(20,20));
        ui->calibItemTableWidget->setItem(i, 3, newItem4);
    }
    //ui->calibItemTableWidget->resizeColumnsToContents();
    ui->calibItemTableWidget->resizeRowsToContents();
}


void CalibForm::onAdcSampleReceived(uint8_t channel, float value)
{
    double ubias = 0.;
    if (channel>=2) {
        QString result = getCalibParameter("VDIV");
        if (result!="") {
            double vdiv = result.toDouble(NULL);
            ubias = vdiv*value/100.;
//            ui->biasVoltageLineEdit->setText(QString::number(ubias));
        }
    }
    if (channel == 3) {
        ui->biasAdcLineEdit->setText(QString::number(value));
        ui->biasVoltageLineEdit->setText(QString::number(ubias));
        if (fCalibRunning) {
            if (fCurrBias>calVoltMax) { on_doBiasCalibPushButton_clicked(); return; }
            QPointF p(fCurrBias, ubias);
            fCurrBias+=BIAS_SCAN_INCREMENT;
            emit setBiasDacVoltage(fCurrBias);
            QThread::msleep(100);
            fPoints2.push_back(p);
            ui->biasVoltageCalibPlot->curve("curve2").setSamples(fPoints2);

            double vdiv=getCalibParameter("VDIV").toDouble()*0.01;
            double rsense = getCalibParameter("RSENSE").toDouble()*0.1/1000.; // RSense in MOhm
            QPointF p2(ubias,vdiv*(fLastRSenseHiVoltage-value)/rsense);
            fPoints3.push_back(p2);
            ui->biasCurrentCalibPlot->curve("curve3").setSamples(fPoints3);

            ui->biasVoltageCalibPlot->replot();
            ui->biasCurrentCalibPlot->replot();
        }
        if (currentCalibValid()) {
            double ioffs = ubias*fSlope2+fOffs2;

            double vdiv=getCalibParameter("VDIV").toDouble()*0.01;
            double rsense = getCalibParameter("RSENSE").toDouble()*0.1/1000.; // RSense in MOhm
            double ibias = (fLastRSenseHiVoltage-value)*vdiv/rsense-ioffs;
            ui->biasCurrentLineEdit->setText(QString::number(ibias,'f',1)+" uA");
        }
        else {
            double ioffs = 0.;
            double vdiv=getCalibParameter("VDIV").toDouble()*0.01;
            double rsense = getCalibParameter("RSENSE").toDouble()*0.1/1000.; // RSense in MOhm
            double ibias = (fLastRSenseHiVoltage-value)*vdiv/rsense-ioffs;
            ui->biasCurrentLineEdit->setText(QString::number(ibias,'f',1)+" uA");
        }
        fLastRSenseLoVoltage = value;
    } else if (channel == 2) {
        if (fCalibRunning) {
            QPointF p(fCurrBias, ubias);
            fPoints1.push_back(p);
            ui->biasVoltageCalibPlot->curve("curve1").setSamples(fPoints1);
            ui->biasVoltageCalibPlot->replot();
            doFit();
        }
        fLastRSenseHiVoltage = value;
    }
}

void CalibForm::on_readCalibPushButton_clicked()
{
    // calib reread triggered
    emit calibRequest();
}


void CalibForm::on_writeEepromPushButton_clicked()
{
    // write eeprom clicked
    emit writeCalibToEeprom();
}


void CalibForm::on_doBiasCalibPushButton_clicked()
{
    // let's go
    if (fCalibRunning) {
        fCalibRunning=false;
        ui->doBiasCalibPushButton->setText("Start");
        return;
    }
    fCalibRunning=true;
    ui->doBiasCalibPushButton->setText("Stop");
    fPoints1.clear();
    fPoints2.clear();
    fPoints3.clear();
    fCurrBias=calVoltMin;
    emit setBiasDacVoltage(fCurrBias);
}

void CalibForm::doFit()
{
//    double slope1=0.,offs1=0.;
//    double slope2=0.,offs2=0.;
    QVector<QPointF> goodPoints1,goodPoints2, goodPoints3;

    ui->fitTextEdit->clear();

    std::copy_if(fPoints1.begin(), fPoints1.end(), std::back_inserter(goodPoints1), [](const QPointF& p)
            { return p.y()<26. && p.y()>7.0; } );

    bool ok=calcLinearCoefficients(goodPoints1,&fOffs1,&fSlope1);
    if (!ok) return;
    QVector<QPointF> vec;
    QPointF p1,p2;
    p1.rx()=0.;
    p1.ry()=fOffs1;
    p2.rx()=2.2;
    p2.ry()=fOffs1+2.2*fSlope1;
    vec.push_back(p1);
    vec.push_back(p2);
    ui->biasVoltageCalibPlot->curve("Fit1").setSamples(vec);
    ui->biasVoltageCalibPlot->replot();
    ui->fitTextEdit->append("fit voltage: c0="+QString::number(fOffs1)+", c1="+QString::number(fSlope1));

/*
    std::copy_if(fPoints2.begin(), fPoints2.end(), std::back_inserter(goodPoints2), [](const QPointF& p)
            { return p.y()<26. && p.y()>7.0; } );
    ok=calcLinearCoefficients(goodPoints2,&fOffs2,&fSlope2);
    if (!ok) return;
    vec.clear();
    p1.rx()=0.;
    p1.ry()=fOffs2;
    p2.rx()=2.2;
    p2.ry()=fOffs2+2.2*fSlope2;
    vec.push_back(p1);
    vec.push_back(p2);
    ui->biasVoltageCalibPlot->curve("Fit2").setSamples(vec);
    ui->fitTextEdit->append("fit bias2: c0="+QString::number(fOffs2)+", c1="+QString::number(fSlope2));
*/
    // here, we have 2 fits successfully applied. Now we can use the fit fuctions
    // V1=Slope1*VADC1+Offs1
    // V2=Slope2*VADC2+Offs2
    // VADC,Diff=VADC1-VADC2
    // VDiff,corr = VDiff * (func1-func2)

    ui->biasVoltageCalibPlot->replot();

    std::copy_if(fPoints3.begin(), fPoints3.end(), std::back_inserter(goodPoints3), [](const QPointF& p)
            { return p.x()<25. && p.x()>7.0; } );
//    double currOffs, currSlope;
    ok=calcLinearCoefficients(goodPoints3,&fOffs2,&fSlope2);
    if (!ok) return;
    vec.clear();
    p1.rx()=0.;
    p1.ry()=fOffs2;
    p2.rx()=40.;
    p2.ry()=fOffs2+40.0*fSlope2;
    vec.push_back(p1);
    vec.push_back(p2);
    ui->biasCurrentCalibPlot->curve("Fit").setSamples(vec);
    ui->fitTextEdit->append("fit current: c0="+QString::number(fOffs2)+", c1="+QString::number(fSlope2));

    ui->biasCurrentCalibPlot->replot();
    ui->transferBiasCoeffsPushButton->setEnabled(true);

}

void CalibForm::on_transferBiasCoeffsPushButton_clicked()
{
    // transfer to calib
    if (ui->transferBiasCoeffsPushButton->isEnabled()) {
        QVector<CalibStruct> items;
        QString str=QString::number(fOffs1);
        if (str.size()) {
            setCalibParameter("COEFF0",str);
            items.push_back(getCalibItem("COEFF0"));
        }
        str=QString::number(fSlope1);
        if (str.size()) {
            setCalibParameter("COEFF1",str);
            items.push_back(getCalibItem("COEFF1"));
        }
        str=QString::number(fOffs2);
        if (str.size()) {
            setCalibParameter("COEFF2",str);
            items.push_back(getCalibItem("COEFF2"));
        }
        str=QString::number(fSlope2);
        if (str.size()) {
            setCalibParameter("COEFF3",str);
            items.push_back(getCalibItem("COEFF3"));
        }
        uint8_t flags=getCalibParameter("CALIB_FLAGS").toUInt();
        flags |= CalibStruct::CALIBFLAGS_VOLTAGE_COEFFS | CalibStruct::CALIBFLAGS_CURRENT_COEFFS;
        setCalibParameter("CALIB_FLAGS",QString::number(flags));
        items.push_back(getCalibItem("CALIB_FLAGS"));
        updateCalibTable();
        emit updatedCalib(items);
    }
}

void CalibForm::setCalibParameter(const QString &name, const QString &value)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            result->value=value.toStdString();
        }
    }
}

QString CalibForm::getCalibParameter(const QString &name)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            return QString::fromStdString(result->value);
        }
    }
    return "";
}

const CalibStruct& CalibForm::getCalibItem(const QString &name)
{

    if (!fCalibList.empty()) {
        QVector<CalibStruct>::iterator result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            return *result;
        }
    }
    return invalidCalibItem;
}

bool CalibForm::voltageCalibValid()
{
    //
    int calibFlags = getCalibParameter("CALIB_FLAGS").toUInt();
    if (calibFlags & CalibStruct::CALIBFLAGS_VOLTAGE_COEFFS) return true;
    return false;
}

bool CalibForm::currentCalibValid()
{
    //
    int calibFlags = getCalibParameter("CALIB_FLAGS").toUInt();
    if (calibFlags & CalibStruct::CALIBFLAGS_CURRENT_COEFFS) return true;
    return false;
}

void CalibForm::onUiEnabledStateChange(bool connected)
{
    //measureBiasCalibGroupBox
    if (!connected) {
        ui->calibItemTableWidget->setRowCount(0);
        fCalibList.clear();
        ui->eepromValidLabel->setText("EEPROM data: ");
        ui->calibValidLabel->setText("Calib data: ");
        ui->idLineEdit->setText("N/A");
    }
    ui->measureBiasCalibGroupBox->setEnabled(connected);
    ui->biasCalibGroupBox->setEnabled(connected);
    ui->calibItemsGroupBox->setEnabled(connected);
    ui->currentCalibGroupBox->setEnabled(connected);
}


void CalibForm::on_calibItemTableWidget_cellChanged(int row, int column)
{
    //
    if (column==2) {
        QString name=ui->calibItemTableWidget->item(row,0)->text();
        QString valstr=ui->calibItemTableWidget->item(row,2)->text();
        if (valstr=="") { updateCalibTable(); return; }
        bool ok=false;
        double val=valstr.toDouble(&ok);
        if (!ok) { updateCalibTable(); return; }
        setCalibParameter(name,valstr);
        QVector<CalibStruct> items;
        items.push_back(getCalibItem(name));
        emit updatedCalib(items);
    }
}

