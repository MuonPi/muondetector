/***************************************************************************
 *   Copyright (C) 2007 by HG Zaunick   *
 *   zhg@gmx.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QEvent>
#include <QQueue>
#include <QList>
#include <QVector>
#include <QScrollBar>
#include <QPen>
#include <QBitmap>

#include <memory>

#include "QHistogram.h"
#include "QPoint3.h"
#include "func.h"

template class QVector<float>;
class QPaintEvent;
class QPointF;
class QBitmap;
class QDialog;
class MultiIntInputDialog;
class MultiDoubleInputDialog;
//class QHist2d;
class QMenuBar;
class QMenu;
class QAction;
class QActionGroup;
class QStatusBar;
class QLayout;
//template class hgz::Function<double,double>;
//class QPen;

const QEvent::Type PaintLineEventType = (QEvent::Type)10001;
//const QEvent::Type PaintLineEventType = (QEvent::Type)10001;
QRect getOverlap(const QRect& rect1, const QRect& rect2);

class PaintLineEvent : public QEvent
{
   public:
      PaintLineEvent(int line)
         :  QEvent(PaintLineEventType), fLine(line) {}
      void setLine(int line) { fLine = line; }
      int  getLine() const { return fLine; }
   private:
      int fLine;
};






/**
 * @class RenderArea
 * @short base class for graphical display on a widget
 */
class RenderArea : public QWidget
{
   Q_OBJECT

   public:
      RenderArea(QWidget *parent = 0);
      RenderArea(QWidget *parent,
                 QPixmap& pixmap,
                 bool scaled = false);
      ~RenderArea() {}

      void setPixmap(QPixmap& pixmap) { fPixmap = pixmap; }
      bool scaled() const { return fScaled; }
      void setScaled(bool scaled) { fScaled = scaled; }
      const QPixmap& pixmap() const { return fPixmap; }
      QPixmap& pixmap() { return fPixmap; }
      void SetVerbosity(int level) {fVerbose=level;}
      int Verbosity() const {return fVerbose;}

   signals:
      void mouseMove(QMouseEvent*);
   protected:
      void paintEvent(QPaintEvent *event);
      bool event(QEvent *event);

      QPixmap fPixmap;
//      QLayout* fLayout;
      bool fScaled;
      int fVerbose;

   private:
};


/**
 * @class DiagramWidget
 * @short class for display of arbitrary data in a diagram
 * @author HG Zaunick
 * @date 08.01.2010
 */
class DiagramWidget : public RenderArea
{
   Q_OBJECT
   public:
      DiagramWidget(QWidget* parent=NULL);
      ~DiagramWidget() {}
      QRectF bounds() { return fBounds; }
      bool directXMapping() const { return fDirectXMapping; }
//      QPen pen() const { return fPen; }
      QColor bgColor() const { return fBgColor; }
      QColor fgColor() const { return fFgColor; }
      QColor fnColor() const { return fFnColor; }
      double xFactor() const { return fXFactor; }
      bool gridX() const { return fGridX; }
      bool gridY() const { return fGridY; }
      bool labels() const { return fLabels; }
      void enableContextMenu(bool enable) { fContextMenuEnabled = enable; }
      bool contextMenuEnabled() { return fContextMenuEnabled; }
      hgz::Function<double,double>& function() { return *pFunction; }
      void inhibitAutoscale(bool inhibit=true);

   signals:
      void mouseMove(QMouseEvent*);
      void mouseMove(double,double);

   public slots:
      void setData(const QVector<int>& data);
      void setData(const QVector<float>& data);
      void setData(const std::vector<double>& data);
      void setData(const QVector<QPointF>& data);
      /**
       * set visible window bounds
       * @param rect ( min(x), min(y), range(x), range(y) )
       */
      void setBounds(const QRectF& rect);
      void setBounds(double xmin, double xmax, double ymin, double ymax) {
         setBounds(QRectF(xmin,ymin,xmax-xmin,ymax-ymin));
      }
      void setBounds(const QVector<double>& bounds) {
         setBounds(bounds[0],bounds[1],bounds[2],bounds[3]);
      }
      void setDirectXMapping ( bool value = true ) { fDirectXMapping = value; createMask(); }
//      void setPen ( const QPen& value ) { fPen = value; }
      void setBgColor(const QColor& bgColor) { fBgColor = bgColor; }
      void setFgColor(const QColor& fgColor) { fFgColor = fgColor; }
      void setFnColor(const QColor& fnColor) { fFnColor = fnColor; }
      void setXFactor ( double value );
      void setGridX ( bool value ){ fGridX = value; createMask(); }
      void setGridY ( bool value ){ fGridY = value; createMask(); }
      void setLabels ( bool value ) { fLabels = value; createMask(); }
      void setPointSize (float value) { fPointSize=value; update(); }
      void autoscale(bool autoscale=true);
      void connectPoints (bool _connect=true) {fConnectPoints=_connect;draw();}
      void clear();
      void chooseFgColor();
      void chooseBgColor();
      void chooseFnColor();
      void choosePointSize();
      void saveImage(const QString& filename);
      void saveImage();

      void setFunction(const hgz::Function<double,double>& func);

   protected:
      void paintEvent( QPaintEvent *event );
      void resizeEvent( QResizeEvent *event );
      void contextMenuEvent( QContextMenuEvent *event);
      void mouseMoveEvent(QMouseEvent * event);
   private:
      QVector<QPointF> fPointValues;
      QVector<double> fSingleValues;
      //QVector<int> fOldData;
      QRectF fBounds;
//      bool fFitX;
      bool fGridX;
      bool fGridY;
      bool fLabels;
      QBitmap fMask;
//      QPen fPen;
      QColor fBgColor;
      QColor fFgColor;
      QColor fFnColor;
      float fPointSize;
      bool fConnectPoints;
      bool fInhibitAutoscale;
      double fXFactor;
      bool fHasXValues;
      bool fDirectXMapping;
      bool fAutoScale;
      bool fContextMenuEnabled;
      bool fShowFunction;
      /// pointer to user function
      std::auto_ptr<hgz::Function<double,double> > pFunction;
      MultiDoubleInputDialog* fBoundsDialog;
      QStatusBar* fStatusBar;
      QMenuBar* fMenuBar;
      QMenu *contextMenu;
      QMenu *fileMenu;
      QMenu *editMenu;
      QMenu *helpMenu;
      QAction *saveAct;
      QAction *exportAct;
      QAction *autoscaleAct;
      QAction *connectPointsAct;
      QAction *exitAct;
      QAction *boundaryAct;
      QAction *aboutAct;
      QAction *fgColAct;
      QAction *bgColAct;
      QAction *fnColAct;
      QAction *pointSizeAct;
      QAction *aboutQtAct;


      void createActions();
      void createMenus();
      void createStatusBar();
      void draw();
      void createMask();
      double bcdExtend(double number);
      double bcdReduce(double number);
      void adjustBounds();
};


/**
 * @class WaterFallWidget
 * @short class for display of spectral time sequences (FFT Waterfall)
 */
class WaterFallWidget : public RenderArea
{
   Q_OBJECT
   public:
      WaterFallWidget( QWidget* parent = 0,
                       int nrLines = 100,
                       float offset = -120.,
                       float range = 120.);
      ~WaterFallWidget() { /*fQueue.clear();*/ fData.clear(); fImages.clear(); }

      float Offset() const { return fOffset; }
      float Range() const { return fRange; }
      QVector<QRgb> ColorTable() const { return fColorTable; }
      static QVector<QRgb> getRainbowPalette(int size, double start, double range);
   signals:
      void mouseMove(QMouseEvent*);
      void mouseMove(double,double);
   public slots:
      void Add(const QVector<float>& line);
      void clear();
      void setOffset ( float value ) { fOffset = value; }
      void setRange ( float value ) { fRange = value; }
      void setColorTable(const QVector<QRgb>& coltab);
   protected:
      void paintEvent( QPaintEvent *event );
      void resizeEvent( QResizeEvent *event );
      void mouseMoveEvent(QMouseEvent * event);
   private:
//      QQueue<QVector<float> > fQueue;
      QVector<float> fData;
      int fHeight;
      QList<QImage> fImages;
      int fLineCounter;
      int fPicCounter;
      int fPicHeight;
      enum { ColormapSize = 256 };
      QVector<QRgb> fColorTable;
//      uint colormap[ColormapSize];
      float fOffset;
      float fRange;
      double fMinFreq, fMaxFreq;

      void recreateImages(int newWidth);
      static QColor getFalseColor(double val);
      static uint rgbFromWaveLength(double wave);
};


/**
 * @class Histo2dWidget
 * @short class for display of data in a 2D Histogram
 * @author HG Zaunick <zhg@gmx.de>
 * @date 07.01.2010
 */
class Histo2dWidget : public RenderArea
{
   Q_OBJECT
   public:
      Histo2dWidget(QWidget* parent=NULL, int xbins=50, int ybins=50);
      ~Histo2dWidget();
      QRectF bounds() const { return fBounds; }
      int binsX() const { return fXBins; }
      int binsY() const { return fYBins; }
//      QPen Pen() const { return fPen; }
      QColor bgColor() const { return fBgColor; }
      bool gridX() const { return fGridX; }
      bool gridY() const { return fGridY; }
      void fill(double x, double y, double value);
   public slots:
      void setData(const QVector<QPoint3F>& data);
      void setBinsX(int bins);
      void setBinsY(int bins);
      void setBins(int xbins, int ybins);
      void setBins(const QVector<int>& bins);
      void setXMin(double xmin);
      void setXMax(double xmax);
      void setYMin(double ymin);
      void setYMax(double ymax);
      void setBounds(const QRectF& rect);
      void setBounds(double xmin, double xmax, double ymin, double ymax) {
         setBounds(QRectF(xmin,ymin,xmax-xmin,ymax-ymin));
      }
      void setBounds(const QVector<double>& bounds) {
         setBounds(bounds[0],bounds[1],bounds[2],bounds[3]);
      }
//      void setPen ( const QPen& value ) { fPen = value; }
      void setBgColor(const QColor& bgColor) { fBgColor = bgColor; }
      void setGridX ( bool value ){ fGridX = value; createMask(); }
      void setGridY ( bool value ){ fGridY = value; createMask(); }
      void autoscale(bool autoscale);
      void clear();
      void saveImage(const QString& filename);
      void saveImage();
      void exportData();
      void chooseBgColor();
      void chooseGridColor();
   private slots:
      void setPalette();
      void setGradation();
      void invertPalette(bool);
      void interpolateBins(bool);
      void chooseInterpolIterations();
      void showPalette(bool showPalette=true);
      void showGrid(bool showPalette=true);
      void fillHisto();

   protected:
      void closeEvent( QCloseEvent *event );
      void paintEvent( QPaintEvent *event );
      void resizeEvent( QResizeEvent *event );
      void contextMenuEvent( QContextMenuEvent *event);
      void mouseMoveEvent(QMouseEvent * event);
   private:
      QHist2d* fHisto;
      int fXBins, fYBins;
      QVector<QPoint3F> fData;
      QRectF fBounds;
      bool fGridX;
      bool fGridY;
      bool fLabels;
      int fNrXTicks,fNrYTicks;
      QBitmap fMask;
      QPixmap fPalettePixmap;
      int fPaletteWidth;
//      QPen fPen;
      QColor fBgColor;
      QColor fGridColor;
      bool fAutoScale;
      bool fInterpolateBins;
      int fInterpolateIterations;
      bool fShowPalette;
      bool fShowGrid;
      QHist2d::HISTO_PALETTE fHistPalette;
      QHist2d::HISTO_GRADATION fHistGradation;
      bool fHistInvert;
      MultiIntInputDialog* fBinningDialog;
      MultiDoubleInputDialog* fBoundsDialog;
      QMenuBar* fMenuBar;
      QStatusBar* fStatusBar;
      QMenu *contextMenu;
      QMenu *fileMenu;
      QMenu *editMenu;
      QMenu *helpMenu;
      QAction *saveAct;
      QAction *exportAct;
      QAction *autoscaleAct;
      QAction *exitAct;
      QAction *binningAct;
      QAction *boundaryAct;
      QAction *aboutAct;
      QAction *aboutQtAct;
      QAction *greyscaleAct;
      QAction *colorAct;
      QAction *monochrome1Act;
      QAction *monochrome2Act;
      QAction *monochrome3Act;
      QAction *monochrome4Act;
      QAction *monochrome5Act;
      QAction *linearAct;
      QAction *logAct;
      QAction *invertAct;
      QAction *bgColAct;
      QAction *gridColAct;
      QAction *interpolAct;
      QAction *interpolIterationsAct;
      QAction *showPaletteAct;
      QAction *showGridAct;
      QActionGroup *paletteGroup;
      QActionGroup *gradationGroup;

      void createActions();
      void createMenus();
      void createStatusBar();
      void draw();
      void drawPalette();
      void createMask();
      double bcdExtend(double number);
      double bcdReduce(double number);
      void adjustBounds();
};



#endif // RENDERAREA_H
