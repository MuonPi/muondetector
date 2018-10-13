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

#include <QtGui>

#include <iostream>
#include <cmath>
#include <cfloat>

#include "QHistogram.h"
#include "RenderArea.h"
#include "multidialog.h"
#include "func.h"
#include "Array2D.h"


#define XBINS 50
#define YBINS 50
#include "Fits.h"

static const int MAX_PIC_SIZE = 4000;

template < typename T >
T sgn( T const &value )
{
   if (value==T()) return T();
   if (value<T()) return T(-1);
   return T(1);
}

using namespace std;
using namespace hgz;

QRect getOverlap(const QRect& rect1, const QRect& rect2)
{
   QRect result;
   result.setTop(std::max(rect1.top(),rect2.top()));
   result.setLeft(std::max(rect1.left(),rect2.left()));
   result.setBottom(std::min(rect1.bottom(),rect2.bottom()));
   result.setRight(std::min(rect1.right(),rect2.right()));
   if (result.bottom()<result.top()) return QRect();
   if (result.right()<result.left()) return QRect();
   if (result.bottom()<=result.top() && result.right()<=result.left()) return QRect();
   return result;
}

/**
 * create empty object without pixmap object
 * @param parent parent widget
 */
RenderArea::RenderArea(QWidget * parent)
   :  QWidget(parent), fPixmap(QPixmap()), fScaled(false)
{
   fVerbose=0;
   setMouseTracking(true);
}

/**
 * construct object with given QPixmap object
 * @param parent parent widget
 * @param pixmap the pixmap that shall be rendered
 * @param scaled true, if content should be scaled to the actual object size
 */
RenderArea::RenderArea(QWidget* parent,
                       QPixmap& pixmap,
                       bool scaled)
   :  QWidget(parent), fPixmap(pixmap), fScaled(scaled)
{
   fVerbose=0;
   setMouseTracking(true);
}

/**
 * overloaded paintEvent slot
 * @param event
 */
void RenderArea::paintEvent(QPaintEvent * event)
{
   if (fPixmap.isNull()) return;
   QPainter painter(this);

//   std::cout<<"y1 = "<<event->rect().top()<<", y2="<<event->rect().bottom()<<std::endl;

   if (fScaled) {
      painter.scale( (double)this->width()  / (double)fPixmap.width(),
                     (double)this->height() / (double)fPixmap.height() );
      QRect exposedRect = painter.matrix().inverted()
            .mapRect(event->rect())
            .adjusted(-1, -1, 1, 1);
      painter.drawPixmap(exposedRect,fPixmap,exposedRect);
   } else {
      QRect rect = getOverlap(event->rect(),fPixmap.rect());
      painter.drawPixmap(rect,fPixmap,rect);
//      painter.drawPixmap(0,0,fPixmap);
   }
}


bool RenderArea::event(QEvent * event)
{
   return QWidget::event(event);
   QWidget::event(event);

   if (event->type() == PaintLineEventType) {
      PaintLineEvent *myEvent = static_cast<PaintLineEvent *>(event);
//      QPainter painter(this);
      double yscale = (double)this->height() / (double)fPixmap.height();
      double y1 = yscale * (double)myEvent->getLine();
//      double y2 = yscale * (double)(myEvent->getLine());

//      std::cout<<"y1 = "<<y1<<", y2="<<y2<<std::endl;

//      QRect rect(0,(int)(y1),this->width(),0);
//      painter.drawPixmap(rect,*fPixmap,rect);

      QRect rect(0,myEvent->getLine(),fPixmap.width(),1);
//      this->repaint(rect);
      this->update(rect);
//      this->update();
//      this->repaint();

//      std::cout<<" PaintLineEvent called."<<std::endl;
      // custom event handling here

      return true;
   } else if (event->type() == QEvent::MouseMove) {
//      std::cout<<" MouseMoveEvent called."<<std::endl;
//      emit(mouseMove((QMouseEvent*)event));
     
   }
   return QWidget::event(event);
}



// class WaterFallWidget
/**
 * constructor
 * @param parent Parent Object
 * @param nrLines Length of Queue
 * @param offset signal offset (noise floor) in dB
 * @param range signal range in dB
 */
WaterFallWidget::WaterFallWidget(QWidget * parent,
                                 int nrLines,
                                 float offset,
                                 float range)
   :  RenderArea(parent), fHeight(nrLines), fOffset(offset), fRange(range)
{
   fLineCounter = 0;
   fPicCounter = 0;

   fMinFreq = 0.;
   fMaxFreq = this->width();

   this->setAttribute(Qt::WA_PaintOnScreen);
// to nullify small flashes with movement of mouse
   this->setAttribute(Qt::WA_NoSystemBackground);

   recreateImages(this->width());
   fColorTable=getRainbowPalette(256,0.,1.);

   // create Colormap
//   for (int i = 0; i < ColormapSize; ++i)
//      colormap[i] = rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize));

   /*
   QVector<QRgb> coltab;
   for (int i = 0; i < 256; ++i)
   {
      coltab.push_back(QRgb(rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize))));
   }

   for (int pic = 0; pic<fImages.size(); ++pic) {
      fImages[pic].setColorTable(coltab);
   }
   */
   this->clear();
}

/**
 * recreate paint image segments with new size
 * @param newWidth new width of paint images
 */
void WaterFallWidget::recreateImages(int newWidth)
{
   fImages.clear();
   fPicHeight = MAX_PIC_SIZE / newWidth - 1;
   if (!fPicHeight) fPicHeight=1;
   if (fVerbose) std::cout<<"WaterFallWidget::WaterFallWidget() : picture segment height = "<<fPicHeight<<std::endl;
   int nrPics = (fHeight-1)/fPicHeight + 2;

   for (int i = 0; i<nrPics; ++i) {
//      fImages.append(QImage(newWidth,fPicHeight,QImage::Format_RGB32));
      fImages.append(QImage(newWidth,fPicHeight,QImage::Format_Indexed8));
   }
   if (fVerbose) std::cout<<"WaterFallWidget::WaterFallWidget() : picture segment count = "<<fImages.size()<<std::endl;

   this->setColorTable(fColorTable);
}


/**
 * Add new Line to the Widget
 * @param line line of values
 */
void WaterFallWidget::Add(const QVector< float > & line)
{
//   fQueue.enqueue(line);
//   if (fQueue.size()>fHeight) fQueue.dequeue();
   fData = line;

   this->setUpdatesEnabled(false);

   if (++fLineCounter>=fPicHeight) {
      fLineCounter=0;
      fPicCounter = (fPicCounter+1)%fImages.size();
      if (fVerbose>1) std::cout<<" pic counter = "<<fPicCounter<<std::endl;
   }


//   QPainter painter(&fImages[fPicCounter]);
//   uint *ptr = reinterpret_cast<uint *>(fImages[fPicCounter].scanLine(fPicHeight-fLineCounter-1));

//   int _actWidth = std::min(this->width(),fQueue.last().size());
   int _actWidth = std::min(this->width(),fData.size());
   for (int i=0; i < _actWidth; i++)
   {
      float val=fData[i];
      short col = ColormapSize*((val-fOffset)/fRange);
      if (col<0) col=0;
      if (col>=ColormapSize) col=ColormapSize-1;
      fImages[fPicCounter].scanLine(fPicHeight-fLineCounter-1)[i] = col;
   }

   this->setUpdatesEnabled(true);
   update();
}

void WaterFallWidget::mouseMoveEvent(QMouseEvent * event)
{
  QMouseEvent* ev = (QMouseEvent*)event;    
  emit(mouseMove(ev));
  double x=ev->pos().x();
  double y=ev->pos().y();
  emit(mouseMove(x,y));
}


/**
 * overloaded widget paint event handler
 * @param event paint event
 */
void WaterFallWidget::paintEvent(QPaintEvent * event)
{
//   return;

   QRect srcRect;
   QRect targetRect;

   QPainter painter(this);
//   int _actWidth = std::min(this->width(),fWidth);
   int _actWidth = 0;
//   if (fQueue.size()) _actWidth = std::min(this->width(),fQueue.last().size());
//   else _actWidth = this->width();
   if (fData.size()) _actWidth = std::min(this->width(),fData.size());
   else _actWidth = this->width();
   // aktuelles Teilbild
   srcRect = QRect(0,fPicHeight-fLineCounter-1,_actWidth,fLineCounter+1);
   targetRect = QRect(0,0,_actWidth,fLineCounter+1);
   //targetRect = QRect(0,0,this->width(),fLineCounter+1);
   painter.drawImage(targetRect,fImages[fPicCounter],srcRect);

   for (int i = 0; i<fImages.size()-2; ++i) {
      // ganzes Bild
      int actPic = (fPicCounter-i-1);
      if (actPic<0) actPic = fImages.size()+actPic;
      srcRect = QRect(0,0,_actWidth,fPicHeight);
      targetRect = QRect(0,i*fPicHeight+fLineCounter+1,_actWidth,fPicHeight);
      if (targetRect.top()>=this->height()) return;
      painter.drawImage(targetRect,fImages[actPic],srcRect);
   }
   // letztes Teilstueck
   int actPic = (fPicCounter+1)%fImages.size();
   srcRect = QRect(0,0,_actWidth,fPicHeight-fLineCounter);
   targetRect = QRect(0,(fImages.size()-2)*fPicHeight+fLineCounter+1,_actWidth,fPicHeight-fLineCounter);
   if (targetRect.top() >= this->height()) return;
   painter.drawImage(targetRect,fImages[actPic],srcRect);

//   QRect rect = getOverlap(event->rect(),fPixmap.rect());
//   painter.drawPixmap(rect,fPixmap,rect);
}

/**
 * overloaded resize event\n
 * clears queue and begins new waterfall with new width
 * @param event resize event
 */
void WaterFallWidget::resizeEvent(QResizeEvent * event)
{
   static bool initial = true;
   if (fVerbose>2)
   std::cout<<"old size:("<<event->oldSize().width()<<","
            <<event->oldSize().height()
            <<"), new size:("<<event->size().width()<<","
            <<event->size().height()<<")"<<std::endl;

   if (!initial)
      if (event->size().width()==event->oldSize().width()) return;
   fPicCounter=0;
   fLineCounter=0;
   recreateImages(this->width());
   clear();
   initial=false;
   if (fVerbose>2) std::cout<<"WaterFallWidget::resizeEvent() called"<<std::endl;
//   QWidget::resizeEvent(event);
}

/**
 * clear queue and existing waterfall images
 */
void WaterFallWidget::clear()
{
//   fQueue.clear();
   fData.clear();
   fLineCounter = 0;
   fPicCounter = 0;
   for (int i=0; i<fImages.size(); ++i) fImages[i].fill(Qt::black);
}

/**
 * set table of colors used for intensity encoding
 * @param coltab vector of QRgb color values
 */
void WaterFallWidget::setColorTable(const QVector< QRgb > & coltab)
{
   fColorTable = coltab;
   for (int pic = 0; pic<fImages.size(); ++pic) {
      fImages[pic].setColorTable(fColorTable);
   }
   update();
}

QVector< QRgb > WaterFallWidget::getRainbowPalette(int size, double start, double range)
{
   QVector<QRgb> coltab;
   for (int i = 0; i < size; ++i) {
      double val = (double)i/(double)size/range + start;
//      coltab.push_back(QRgb(rgbFromWaveLength(370.0 + (val * 400.0 ))));
      coltab.push_back(QRgb( getFalseColor(val).rgb()));
   }
   return coltab;
}

QColor WaterFallWidget::getFalseColor(double val)
{
   static const int maxintensity=1791;
   int b,g,r;
   int v=(int)((double)maxintensity*val);
   if (v<0) v=0;
//   if (v>maxintensity) v=maxintensity;

   if (v<=255){
      g=0;
      r=0;
      b=v;
   }
   else if (v>255 && v<=511){
      b=255;
      r=0;
      g=v-256;
   }
   else if (v>511 && v<=767){
      r=0;
      g=255;
      b=767-v;
   }
   else if (v>767 && v<=1023){
      g=255;
      b=0;
      r=v-768;
   }
   else if (v>1023 && v<=1279){
      g=1279-v;
      b=0;
      r=255;
   }
   else if (v>1279 && v<=1535){
      r=255;
      g=0;
      b=v-1280;
   }
   else if (v>1535 && v<=1791){
      r=255;
      g=v-1536;
      b=255;
   }
   else { r=g=b=255; }

   return QColor(r,g,b);
}

uint WaterFallWidget::rgbFromWaveLength(double wave)
{
   double r = 0.0;
   double g = 0.0;
   double b = 0.0;

   if (wave >= 380.0 && wave <= 440.0) {
      r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
      b = 1.0;
   } else if (wave >= 440.0 && wave <= 490.0) {
      g = (wave - 440.0) / (490.0 - 440.0);
      b = 1.0;
   } else if (wave >= 490.0 && wave <= 510.0) {
      g = 1.0;
      b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
   } else if (wave >= 510.0 && wave <= 580.0) {
      r = (wave - 510.0) / (580.0 - 510.0);
      g = 1.0;
   } else if (wave >= 580.0 && wave <= 645.0) {
      r = 1.0;
      g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
   } else if (wave >= 645.0 && wave <= 780.0 ) {
      r = 1.0;
   }


   double s = 1.0;


   if (wave > 700.0)
      s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
   else
   if (wave <  420.0)
      s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);

   r = pow(r * s, 0.8);
   g = pow(g * s, 0.8);
   b = pow(b * s, 0.8);
   return qRgb(int(r * 255), int(g * 255), int(b * 255));
}







// class DiagramWidget
/**
 * creates object with given parent widget
 * @param parent
 */
DiagramWidget::DiagramWidget(QWidget * parent)
   :  RenderArea(parent)
{
   if (parent==NULL) {
      this->setMinimumWidth(300);
      this->setMinimumHeight(250);
   }
   fBounds = QRectF(0.,0.,1.,1.);
   fXFactor = 1.;
   fGridX = true;
   fGridY = true;
   fLabels = true;
   fHasXValues = false;
   fDirectXMapping = false;
   fInhibitAutoscale = false;
   fAutoScale = true;
   fSingleValues.clear();
   fPointValues.clear();
   fBgColor = Qt::black;
   fFgColor = Qt::cyan;
   fFnColor = Qt::green;
   fPointSize = 1.0;
   fConnectPoints=true;
   fMenuBar=NULL;
   fContextMenuEnabled=true;
   pFunction.reset(NULL);
   fShowFunction = false;

   fVerbose=0;

   fBoundsDialog = new MultiDoubleInputDialog(this,4);
   fBoundsDialog->setWindowTitle("Boundaries");
   fBoundsDialog->setLabelText(0,"X Min");
   fBoundsDialog->setLabelText(1,"X Max");
   fBoundsDialog->setLabelText(2,"Y Min");
   fBoundsDialog->setLabelText(3,"Y Max");
   fBoundsDialog->setMinimum(-1e+38);
   fBoundsDialog->setMaximum(+1e+38);
   fBoundsDialog->setValue(0, fBounds.left());
   fBoundsDialog->setValue(1, fBounds.right());
   fBoundsDialog->setValue(2, fBounds.top());
   fBoundsDialog->setValue(3, fBounds.bottom());

   connect(fBoundsDialog, SIGNAL(valuesChanged(QVector<double>)), this, SLOT(setBounds(QVector<double>)));
   if (fVerbose) std::cout<<"created bounds dialog"<<std::endl;

   QGridLayout* layout;
   if (parent==NULL) layout = new QGridLayout;
   if (parent==NULL) layout->setRowStretch(0,1);
   if (parent==NULL) setLayout(layout);

   createActions();
   createMenus();
   createStatusBar();

   if (parent==NULL) layout->setContentsMargins(0,fMenuBar->height(),0,0);
   if (parent==NULL) layout->setVerticalSpacing(0);
//   setLayout(layout);

   setMouseTracking(true);

   int w=this->width();
   int h=this->height();
   if (fVerbose>1) std::cout<<"parent="<<hex<<parent<<std::endl;
   if (parent==NULL) h-=fMenuBar->height()+fStatusBar->height();
   fMask = QBitmap(w,h);
   fPixmap = QPixmap(w,h);
   fPixmap.fill(fBgColor);
   if (fGridX || fGridY) createMask();
   draw();

   //this->setAttribute(Qt::WA_PaintOnScreen);
   //to nullify small flashes with movement of mouse
   //this->setAttribute(Qt::WA_NoSystemBackground);
}

void DiagramWidget::inhibitAutoscale(bool inhibit)
{
  fInhibitAutoscale=inhibit; 
  if (inhibit && fAutoScale) this->autoscale(false);
  autoscaleAct->setEnabled(!inhibit);
}


/**
 * assign new data set to widget
 * @param data vector of data values
 */
void DiagramWidget::setData(const QVector< float > & data)
{
//   clear();
   fPointValues.clear();
   fSingleValues.clear();
   fSingleValues.reserve(data.size());
   for (int i=0; i<data.size(); ++i) {
      fSingleValues.push_back(data[i]);
   }
   if (fHasXValues) {
      fHasXValues = false;
      createMask();
   }
//   this->resize(this->width(),this->height());
   if (fAutoScale) autoscale(fAutoScale);
   else draw();
}

void DiagramWidget::setData(const std::vector<double> & data)
{
//   clear();
   fPointValues.clear();
   fSingleValues.clear();
//   fSingleValues.reserve(data.size());
   fSingleValues = QVector<double>::fromStdVector(data);
//   this->resize(this->width(),this->height());
   if (fHasXValues) {
      fHasXValues = false;
      createMask();
   }
   if (fAutoScale) autoscale(fAutoScale);
   else draw();
}

void DiagramWidget::setData(const QVector< int > & data)
{
//   clear();
   fPointValues.clear();
   fSingleValues.clear();
   fSingleValues.reserve(data.size());
   for (int i=0; i<data.size(); ++i) {
      fSingleValues.push_back(data[i]);
   }
   if (fHasXValues) {
      fHasXValues = false;
      createMask();
   }
//   this->resize(this->width(),this->height());
   if (fAutoScale) autoscale(fAutoScale);
   else draw();
}

void DiagramWidget::setData(const QVector< QPointF > & data)
{
//   clear();
   fSingleValues.clear();
   fPointValues=data;
   if (!fHasXValues) {
      fHasXValues = true;
      createMask();
   }
//   this->resize(this->width(),this->height());
   if (fAutoScale) autoscale(fAutoScale);
   else draw();
//   update();
}


/**
 * custom paint event handler
 * @param event
 */
void DiagramWidget::paintEvent(QPaintEvent * event)
{
   int ystart=0;
   QPainter painter(this);
   if (fMenuBar) ystart=fMenuBar->height();
   painter.drawPixmap(0,ystart,fPixmap);
}

/**
 * custom resize event handler
 * @param event
 */
void DiagramWidget::resizeEvent(QResizeEvent * event)
{
   int w=this->width();
   int h=this->height();
   if (fMenuBar) h-=fMenuBar->height()+fStatusBar->height();
   fPixmap = QPixmap(w,h);
//   fPixmap.fill(fBgColor);
   fMask = QBitmap(w,h);
   //fMask.clear();
   if (fVerbose) std::cout<<" created mask: w="<<fMask.width()<<" h="<<fMask.height()<<std::endl;
//   if (fGridX || fGridY) createMask();
   createMask();
   draw();
   //update();
}

void DiagramWidget::contextMenuEvent(QContextMenuEvent * event)
{
   if (!fContextMenuEnabled) return;
   if (fVerbose) std::cout<<"context menu event"<<std::endl;


   //fBinningDialog->show();

/*
   QWidgetAction* w_act=new QWidgetAction(this);
//   QSpinBox* spinbox = new QSpinBox(this);
   QSlider* slider = new QSlider(this);
   slider->setOrientation(Qt::Horizontal);
   w_act->setDefaultWidget(slider);
   menu.addAction(w_act);
*/
   contextMenu->exec(event->globalPos());
}

void DiagramWidget::mouseMoveEvent(QMouseEvent * event)
{
   //if (parent()) return;
   if (fVerbose>1) std::cout<<"mouseMoveEvent occured"<<std::endl;
   if (fVerbose>1) std::cout<<"x="<<event->pos().x()<<",y="<<event->pos().y()<<std::endl;
   int i=event->pos().x();
   int j=event->pos().y();
   if (parent()==NULL) j-=fMenuBar->height()/*+fStatusBar->height()*/;
   if (fPixmap.rect().contains(QPoint(i,j)))
   {
      double x=(double)i*(((!fDirectXMapping)?fBounds.width():fMask.width())*fXFactor)/fMask.width()+fBounds.left()*fXFactor;
//      double x=(double)i/fPixmap.width()*fBounds.width()+fBounds.left();
      double y=(fPixmap.height()-(double)j)/fPixmap.height()*fBounds.height()+fBounds.top();
/*      std::cout<<"i="<<i<<", fPixmap.width()="<<fPixmap.width()<<
      ",fBounds.width()="<<fPixmap.width()<<", fBounds.left()"<<fBounds.left()<<std::endl;
*/
      if (!parent()) fStatusBar->showMessage("x="+QString::number(x)+" y="+QString::number(y));
      emit mouseMove(x,y);
   }
}

/**
 * set visible window bounds
 * @param rect ( min(x), min(y), range(x), range(y) )
 */
void DiagramWidget::setBounds(const QRectF & rect)
{
   fBounds = rect;
   adjustBounds();

   fBoundsDialog->setValue(0, fBounds.left());
   fBoundsDialog->setValue(1, fBounds.right());
   fBoundsDialog->setValue(2, fBounds.top());
   fBoundsDialog->setValue(3, fBounds.bottom());

   if (fVerbose>1) std::cout<<"set bounds to ("<<rect.left()<<","<<rect.top()<<","<<rect.right()<<","<<rect.bottom()<<")"<<std::endl;

   createMask();
   draw();
   update();
}

void DiagramWidget::autoscale(bool autoscale)
{
//   if (autoscale==fAutoScale) return;
   if (fInhibitAutoscale && autoscale /*&& !fAutoScale*/ ) return;
   autoscaleAct->setChecked(autoscale);
   fAutoScale=autoscale;
   if (!fSingleValues.size() && !fPointValues.size()) return;
	if (!fAutoScale) return;

   double x_min=1e+38;
   double x_max=-1e+38;
   double y_min=1e+38;
   double y_max=-1e+38;

   // Min/Max x+y
   if (fHasXValues) {
      for (int i=0; i<fPointValues.size(); i++)
      {
         double x=fPointValues[i].x();
         double y=fPointValues[i].y();
         if (x<x_min) x_min=x;
         if (x>x_max) x_max=x;
         if (y<y_min) y_min=y;
         if (y>y_max) y_max=y;
      }
   } else {
      x_min=0.;
      x_max=fSingleValues.size();
      for (int i=0; i<fSingleValues.size(); i++)
      {
         double y=fSingleValues[i];
         if (y<y_min) y_min=y;
         if (y>y_max) y_max=y;
      }
   }
   if (fVerbose) std::cout<<"autoscale"<<std::endl;
   if (fVerbose) std::cout<<" xmin="<<x_min<<" xmax="<<x_max<<" ymin="<<y_min<<" ymax="<<y_max<<std::endl;
   setBounds(QRectF(x_min-0.05*(x_max-x_min),
   					  y_min-0.05*(y_max-y_min),
   					  1.1*(x_max-x_min),
   					  1.1*(y_max-y_min)));
}

void DiagramWidget::adjustBounds()
{
   if (fBounds.left()==fBounds.right()) {
      if (fBounds.left()==0.) {
         fBounds.setLeft(-0.1);
         fBounds.setRight(0.1);
      } else {
         fBounds.setLeft(fBounds.left()-0.1*fabs(fBounds.left()));
         fBounds.setRight(fBounds.right()+0.1*fabs(fBounds.right()));
      }
      if (fVerbose>1) {
         cout<<"modified x-bounds to ("<<fBounds.left()<<","<<fBounds.right()<<")"<<endl;
      }
   }

   if (fBounds.top()==fBounds.bottom()) {
      if (fBounds.top()==0.) {
         fBounds.setTop(-0.1);
         fBounds.setBottom(0.1);
      } else {
         fBounds.setTop(fBounds.top()-0.1*fabs(fBounds.top()));
         fBounds.setBottom(fBounds.bottom()+0.1*fabs(fBounds.bottom()));
      }
      if (fVerbose>1) {
         cout<<"modified y-bounds to ("<<fBounds.top()<<","<<fBounds.bottom()<<")"<<endl;
      }
   }
}

/**
 * main draw function of widget. draws data on internal pixmap and sets grid mask
 */
void DiagramWidget::draw()
{
   setUpdatesEnabled(false);
   fPixmap.fill(fBgColor);
   int _useLast = (fConnectPoints)?1:0;
   try {
      QPainter painter(&fPixmap);
      painter.setPen(QPen(fFgColor, fPointSize));
      //QPen delPen(fPen);
      //delPen.setColor(Qt::black);
   //      fPixmap.fill(Qt::black);
         //QPainterPath path;
         //path.moveTo(0,0);
         if (fDirectXMapping) {
            if (fHasXValues) {
               for (int i=0; i<std::min(fPointValues.size(),fPixmap.width())-_useLast; i++) {
                  int y1=fPixmap.height()*(1.-(fPointValues[i].y()-fBounds.top())/(fBounds.height()));
                  int y2=fPixmap.height()*(1.-(fPointValues[i+_useLast].y()-fBounds.top())/(fBounds.height()));
                  //if (y>=0 && y<fPixmap.height())
		  {
                     //if (i==0) painter.drawPoint(i,y);
                     //else
		       if (fConnectPoints) painter.drawLine(i,y1,i+1,y2);
		       else if (y1>=0 && y1<fPixmap.height())
		       painter.drawPoint(i,y1);
                  }
               }
            } else {
               for (int i=0; i<std::min(fSingleValues.size(),fPixmap.width())-_useLast; i++) {
                  int y1=fPixmap.height()*(1.-(fSingleValues[i]-fBounds.top())/(fBounds.height()));
                  int y2=fPixmap.height()*(1.-(fSingleValues[i+_useLast]-fBounds.top())/(fBounds.height()));
                  //if (y>=0 && y<fPixmap.height())
		  {
                     if (fConnectPoints) painter.drawLine(i,y1,i+1,y2);
		       else if (y1>=0 && y1<fPixmap.height())
		       painter.drawPoint(i,y1);
//                     painter.drawPoint(i,y);
                  }
               }
            }
         } else {
            if (fHasXValues) {
               for (int i=0; i<fPointValues.size()-_useLast; i++) {
                  int x1=fPixmap.width()*(fPointValues[i].x()-fBounds.left())/(fBounds.width());
                  int x2=fPixmap.width()*(fPointValues[i+_useLast].x()-fBounds.left())/(fBounds.width());
   //            int x=this->width()*i/fData.size();

                  int y1=fPixmap.height()*(1.-(fPointValues[i].y()-fBounds.top())/(fBounds.height()));
                  int y2=fPixmap.height()*(1.-(fPointValues[i+_useLast].y()-fBounds.top())/(fBounds.height()));
               //if (i==0) { path.moveTo(x,y); continue; }
                  if (!fConnectPoints)
		  {
		    if (x1>=0 && x1<fPixmap.width() && y1>=0 && y1<fPixmap.height())
		    {
		      /*
		      painter.setPen(Qt::black);
		      painter.drawPoint(x,y);
		      painter.setPen(fPen);
		      */
		      //path.lineTo(x,y);
		      painter.drawPoint(x1,y1);
		    }
		    //painter.drawPath(path);
		  } else {
		    painter.drawLine(x1,y1,x2,y2);
		  }
		}
            }
            else {
	        int _interval=std::max(fSingleValues.size()/fPixmap.width()/2,1);
               for (int i=0; i<fSingleValues.size()-_useLast; i+=_interval) {
   //            for (int i=0; i<fData.size(); i++) {
                  //int x=this->width()*(fData[i].x()-fBounds.left())/(fBounds.width());
                  int x1=fPixmap.width()*i/fSingleValues.size();
                  int x2=fPixmap.width()*(i+_interval)/fSingleValues.size();
		  if (x2>fBounds.right()) x2=fBounds.right();

                  int y1=fPixmap.height()*(1.-(fSingleValues[i]-fBounds.top())/(fBounds.height()));
                  int y2;
		  if ((i+_interval)>fSingleValues.size())		    		y2=fPixmap.height()*(1.-(fSingleValues[fSingleValues.size()-1]-fBounds.top())/(fBounds.height()));
		  else y2=fPixmap.height()*(1.-(fSingleValues[i+_interval]-fBounds.top())/(fBounds.height()));

//		  cout<<" x1="<<x1<<" x2="<<x2<<" y1="<<y1<<" y2="<<y2<<endl;
		  
		  if (!fConnectPoints)
		  {
		    if (x1>=0 && x1<fPixmap.width() && y1>=0 && y1<fPixmap.height())
		    {
		      /*
		      painter.setPen(Qt::black);
		      painter.drawPoint(x,y);
		      painter.setPen(fPen);
		      */
		      //path.lineTo(x,y);
		      painter.drawPoint(x1,y1);
		    }
		    //painter.drawPath(path);
		  } else {
		    painter.drawLine(x1,y1,x2,y2);
		  }
               }
            }
         }
         if (pFunction.get()!=NULL) {
            // draw function
            painter.setPen(QPen(fFnColor, 1.));
            if (fDirectXMapping) {

               if (fHasXValues) {
               } else {

               }
            } else {
               if (fHasXValues) {
                  for (double t=fBounds.left(); t<fBounds.right(); t+=fBounds.width()/1000.) {
                     int x=fPixmap.width()*(t-fBounds.left())/(fBounds.width());
                     int y=fPixmap.height()*(1.-((*pFunction)(t)-fBounds.top())/(fBounds.height()));

                     //if (i==0) { path.moveTo(x,y); continue; }
                     if (x>=0 && x<fPixmap.width() && y>=0 && y<fPixmap.height())
                     {
                        /*
                        painter.setPen(Qt::black);
                        painter.drawPoint(x,y);
                        painter.setPen(fPen);
                        */
                        //path.lineTo(x,y);
                        painter.drawPoint(x,y);
                     }
                  }
               } else {

               }
            }
         }
         painter.end();
         if ((fGridX || fGridY) && fMask.rect() == fPixmap.rect()) {
         //createMask();
            fPixmap.setMask(fMask);
         }
   }
   catch (...) { cout<<"unhandled exception in DiagramWidget::draw()"<<endl; }
   setUpdatesEnabled(true);
   update();
}

double DiagramWidget::bcdExtend(double number)
{
   if (number==0.) return number;
   double expo = pow(10.,floor(log10(fabs(number))));
   if (fVerbose>1) std::cout<<" expo = "<<expo<<std::endl;
   double base = fabs(number/expo);
   if (fVerbose>1) std::cout<<" base = "<<base<<std::endl;
   base = std::floor(base+0.5);
   int iBase = (int)base;
   if (fVerbose>1) std::cout<<" iBase = "<<iBase<<std::endl;
   switch (iBase)
   {
      case 1 : base = 2.;
               break;
      case 2 :
      case 3:
      case 4:  base = 5.;
               break;
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:  base = 10.;
               break;
      case 10: base = 20.; // eigentlich unnötig, oder?
               break;
      default: return number;
   }
   if (fVerbose>1) std::cout<<"extended to base = "<<base<<std::endl;
   double result = fabs(base*expo);
   if (fVerbose>1) std::cout<<"extended to "<<result<<std::endl;
   return (result > 0) ? result : -result;
}

double DiagramWidget::bcdReduce(double number)
{
   if (number==0.) return number;
   double expo = pow(10.,floor(log10(fabs(number))));
   double base = fabs(number/expo);
   if (fVerbose>1) std::cout<<" base = "<<base<<std::endl;
   base = std::floor(base+0.5);
   int iBase = (int)base;
   switch (iBase)
   {
      case 1 : base = 0.5;
               break;
      case 2 : base = 1.;
               break;
      case 3:
      case 4:
      case 5:  base = 2.;
               break;
      case 6:
      case 7:
      case 8:
      case 9:
      case 10: base = 5.; // eigentlich unnötig, oder?
               break;
      default: return number;
   }
   double result = fabs(base*expo);
   return (result > 0) ? result : -result;
}

/**
 * tbd
 */
void DiagramWidget::createMask()
{
   if (fMask.rect() != fPixmap.rect()) return;
//   fMask.clear();
   fMask.fill(Qt::color1);

   QPainter painter(&fMask);
   QPen pen(Qt::color0);
   pen.setStyle(Qt::DotLine);

   painter.setPen(pen);
   QFont sansFont("Helvetica", 9, QFont::Bold);
//   QFont sansFont("Helvetica [Cronyx]", 8);
   painter.setFont(sansFont);

   // vertical axis
   if (fGridY) {
      int maxTicks = std::max(fMask.height()/50,9);
      if (fVerbose>1) std::cout<<"max. nr. y-ticks : "<<maxTicks<<std::endl;
      //int maxTicks = 8;
      int minTicks = 4;

      double interval = pow(10.,std::floor(log10(fabs(fBounds.height())))-1.);
      if (fVerbose>1) std::cout<<"y-tick interval(0) : "<<interval<<std::endl;
      int nrTicks = fabs(fBounds.height())/interval;
      if (fVerbose>1) std::cout<<"nr. of y-ticks(0) : "<<nrTicks<<std::endl;
      while (nrTicks < minTicks || nrTicks > maxTicks) {
         if (nrTicks < minTicks) interval = bcdReduce(interval);
         else if (nrTicks > maxTicks) interval = bcdExtend(interval);
         else break;
         nrTicks = fabs(fBounds.height())/interval;
         if (fVerbose>1) std::cout<<"y-tick interval : "<<interval<<std::endl;
      }
      if (fVerbose>1) std::cout<<"y-tick interval : "<<interval<<std::endl;
      if (fVerbose>1) std::cout<<"nr of y-ticks : "<<nrTicks<<std::endl;
      double tickLevel = interval*std::floor((double)fBounds.top()/interval+0.5);
      if (fVerbose>1) std::cout<<"first y-tick : "<<tickLevel<<std::endl;

      for (int i=0; i<nrTicks; i++)
      {
         int precision = -(std::floor(log10(interval)));
         //   if (precision<0) precision = 0;
         if (fVerbose>1) std::cout<<"x precision = "<<precision<<std::endl;
         int y=fMask.height()*(1.-(tickLevel-fBounds.top())/(fBounds.height()));
         painter.drawLine(QPoint(0,y),QPoint(fMask.width()-1,y));

//         painter.drawText(QPointF(this->width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel,'g',(precision>0)?precision:0));

         if (fLabels) {
            if (fabs(tickLevel) < 1000.) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel,'f',(precision>0)?precision:0));
            } else if (fabs(tickLevel) < 1e+6) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1000.,'f',(precision+3>0)?precision+3:0)+"k");
            } else if (fabs(tickLevel) < 1e+9) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1e+6,'f',(precision+6>0)?precision+6:0)+"M");
            } else {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1e+9,'f',(precision+9>0)?precision+9:0)+"G");
            }
         }

         tickLevel+=sgn(fBounds.height())*interval;
      }
   }

   if (fGridX) {
      // horizontal axis
      int maxTicks = std::max(fMask.width()/50,10);
      if (fVerbose>1) std::cout<<"max. nr. x-ticks : "<<maxTicks<<std::endl;
      int minTicks = 4;

      if (fVerbose>1) {
         if (fHasXValues) std::cout<<"fPointValues.size()="<<fPointValues.size()<<std::endl;
         else std::cout<<"fSingleValues.size()="<<fSingleValues.size()<<std::endl;
      }
      double interval = pow(10.,std::floor(log10(fabs(fBounds.width()*fXFactor)))-1.);
      if (fVerbose>1) std::cout<<"x-tick interval(0) : "<<interval<<std::endl;
      int nrTicks = fabs(((!fDirectXMapping)?fBounds.width():fMask.width())*fXFactor)/interval;
      if (fVerbose>1) std::cout<<"nr. of x-ticks(0) : "<<nrTicks<<std::endl;
      while (nrTicks < minTicks || nrTicks > maxTicks) {
         if (nrTicks < minTicks) interval = bcdReduce(interval);
         else if (nrTicks > maxTicks) interval = bcdExtend(interval);
         else break;
         nrTicks = fabs(((!fDirectXMapping)?fBounds.width():fMask.width())*fXFactor)/interval;
         if (fVerbose>1) std::cout<<"x-tick interval : "<<interval<<std::endl;
      }
      if (fVerbose>1) std::cout<<"x-tick interval : "<<interval<<std::endl;
      if (fVerbose>1) std::cout<<"nr of x-ticks : "<<nrTicks<<std::endl;
      double tickLevel = interval*std::floor((double)fBounds.left()*fXFactor/interval+0.5);
      if (fVerbose>1) std::cout<<"first x-tick : "<<tickLevel<<std::endl;

      int precision = -(std::floor(log10(interval)));
   //   if (precision<0) precision = 0;
      if (fVerbose>1) std::cout<<"precision = "<<precision<<std::endl;
      for (int i=0; i<nrTicks; i++)
      {
         int x=fMask.width()*((tickLevel-fBounds.left()*fXFactor)/(((!fDirectXMapping)?fBounds.width():fMask.width())*fXFactor));
         painter.drawLine(QPoint(x,0),QPoint(x,fMask.height()-1));

         if (fLabels) {
            if (fabs(tickLevel) < 1000.) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel,'f',(precision>0)?precision:0));
            } else if (fabs(tickLevel) < 1e+6) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1000.,'f',(precision+3>0)?precision+3:0)+"k");
            } else if (fabs(tickLevel) < 1e+9) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1e+6,'f',(precision+6>0)?precision+6:0)+"M");
            } else {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1e+9,'f',(precision+9>0)?precision+9:0)+"G");
            }
         }
         tickLevel+=sgn(fBounds.width())*interval;
      }
   }
}

/**
 * tbd
 */
void DiagramWidget::setXFactor(double value)
{
   fXFactor = value;
//   draw();
//   fPixmap.fill(Qt::black);
   //fOldData.clear();
   createMask();
}

/**
 * clear all data and formatting of the widget
 */
void DiagramWidget::clear()
{
//   fMask = QBitmap(fMask.width(),fMask.height());
   fPixmap = QPixmap(fPixmap.width(), fPixmap.height());
//   fPixmap.fill(fBgColor);
//   fMask.fill(Qt::color1);
//   createMask();
//   fPixmap.setMask(fMask);


//   fMask = QBitmap();
//   fPixmap = QPixmap();
   //fBounds = QRectF(0.,0.,1.,1.);
   //fXFactor = 1.;
   //fGridX = true;
   //fGridY = true;
//   fNrXTicks = -1;
//   fNrYTicks = -1;
   fSingleValues.clear();
   fPointValues.clear();
   fHasXValues = false;
//   setBounds(fBounds);
//   draw();
}

void DiagramWidget::createActions()
{
      saveAct = new QAction(QIcon(":/filesave.xpm"), tr("&Save"), this);
      saveAct->setShortcut(tr("Ctrl+S"));
      saveAct->setStatusTip(tr("Save Image to disk"));
      connect(saveAct, SIGNAL(triggered()), this, SLOT(saveImage()));

      exportAct = new QAction(tr("&Export data"), this);
      exportAct->setStatusTip(tr("export data to ASCII list"));
      connect(exportAct, SIGNAL(triggered()), this, SLOT());

      exitAct = new QAction(tr("C&lose"), this);
      exitAct->setShortcut(tr("Ctrl+Q"));
      exitAct->setStatusTip(tr("Close the window"));
      connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

      aboutAct = new QAction(tr("&About"), this);
      aboutAct->setStatusTip(tr("Show the application's About box"));
      connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

      aboutQtAct = new QAction(tr("About &Qt"), this);
      aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
      connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

      autoscaleAct=new QAction("&Autoscale",this);
      autoscaleAct->setStatusTip(tr("automatically scale the diagrams boundaries"));
      autoscaleAct->setCheckable(true);
      autoscaleAct->setChecked(fAutoScale);
      connect(autoscaleAct, SIGNAL(toggled(bool)), this, SLOT(autoscale(bool)));

      connectPointsAct=new QAction("&Connect Points",this);
      connectPointsAct->setStatusTip(tr("connect data points with straight lines"));
      connectPointsAct->setCheckable(true);
      connectPointsAct->setChecked(fConnectPoints);
      connect(connectPointsAct, SIGNAL(toggled(bool)), this, SLOT(connectPoints(bool)));
      
      boundaryAct=new QAction("B&oundaries",this);
      connect(boundaryAct, SIGNAL(triggered()), fBoundsDialog, SLOT(show()));

      fgColAct = new QAction(tr("&Point Color"), this);
      fgColAct->setStatusTip(tr("Set the Foreground Color"));
      connect(fgColAct, SIGNAL(triggered()), this, SLOT(chooseFgColor()));

      bgColAct = new QAction(tr("&Background Color"), this);
      bgColAct->setStatusTip(tr("Set the Background Color"));
      connect(bgColAct, SIGNAL(triggered()), this, SLOT(chooseBgColor()));

      fnColAct = new QAction(tr("&Function Color"), this);
      fnColAct->setStatusTip(tr("Set the User Function Color"));
      connect(fnColAct, SIGNAL(triggered()), this, SLOT(chooseFnColor()));

      pointSizeAct=new QAction("&Point Size",this);
      connect(pointSizeAct, SIGNAL(triggered()), this, SLOT(choosePointSize()));
}

void DiagramWidget::createMenus()
{
   if (parent()==NULL) {
      fMenuBar = new QMenuBar(this);
      if (fVerbose>2) std::cout<<"created menubar"<<std::endl;
   //   menuBar->setFrameStyle(QFrame::Panel | QFrame::Raised);
      //menuBar->setStyle(new QMotifStyle());
   //   layout->addWidget(menuBar,0,0);
      //layout->setRowStretch(0,0);

      fileMenu = fMenuBar->addMenu(tr("&File"));
      fileMenu->addAction(saveAct);
      fileMenu->addAction(exportAct);
      fileMenu->addSeparator();
      fileMenu->addAction(exitAct);

      editMenu = fMenuBar->addMenu(tr("&Edit"));
      editMenu->addAction(autoscaleAct);
      editMenu->addAction(connectPointsAct);
      editMenu->addAction(pointSizeAct);
      editMenu->addSeparator();
      editMenu->addAction(boundaryAct);
      editMenu->addSeparator();
      editMenu->addAction(fgColAct);
      editMenu->addAction(bgColAct);
      editMenu->addAction(fnColAct);

      fMenuBar->addSeparator();

      helpMenu = fMenuBar->addMenu(tr("&Help"));
      helpMenu->addAction(aboutAct);
      helpMenu->addAction(aboutQtAct);
   }

   contextMenu = new QMenu("context menu",this);
   contextMenu->addAction(autoscaleAct);
   contextMenu->addAction(connectPointsAct);
   contextMenu->addAction(pointSizeAct);
   contextMenu->addSeparator();
   contextMenu->addAction(boundaryAct);
   contextMenu->addSeparator();
   contextMenu->addAction(fgColAct);
   contextMenu->addAction(bgColAct);
   contextMenu->addAction(fnColAct);
   contextMenu->addSeparator();
   contextMenu->addAction(saveAct);
   contextMenu->addAction(exportAct);
   contextMenu->addSeparator();
}

void DiagramWidget::createStatusBar()
{
   if (parent()) return;
   fStatusBar = new QStatusBar(this);
   //statusBar->setMaximumHeight(16);
   //statusBar->setMinimumHeight(16);
   ((QGridLayout*)layout())->addWidget(fStatusBar,1,0);
   ((QGridLayout*)layout())->setRowStretch(1,0);
   fStatusBar->sizePolicy().setVerticalPolicy(QSizePolicy::Fixed);
//   fStatusBar->showMessage(tr("Ready"));
   //statusBar->addWidget(new QFrame(this));
   //statusBar->addWidget(new QProgressBar);
   fStatusBar->setSizeGripEnabled(true);
}

void DiagramWidget::chooseFgColor()
{
   QColor col=QColorDialog::getColor(fFgColor,this);
   if (col.isValid()) { fFgColor=col; draw(); }
}

void DiagramWidget::chooseBgColor()
{
   QColor col=QColorDialog::getColor(fBgColor,this);
   if (col.isValid()) { fBgColor=col; draw(); }
}

void DiagramWidget::chooseFnColor()
{
   QColor col=QColorDialog::getColor(fFnColor,this);
   if (col.isValid()) { fFnColor=col; draw(); }
}

void DiagramWidget::choosePointSize()
{
  bool ok;
  int i = QInputDialog::getInt(this, tr("Data Point Size"),
                                  tr("Percentage:"), fPointSize*10, 0, 100, 1, &ok);
  if (ok)
  {
    fPointSize = (double)i/10.;
    draw();
  }
}


void DiagramWidget::saveImage(const QString & filename)
{
   fPixmap.save(filename,0,100);
}

void DiagramWidget::saveImage()
{
   QString* filter=new QString("*.png");
   QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                            ".", tr("Images (*.png *.xpm *.jpg)"), filter);
   if (filename.size()) saveImage(filename);
   delete filter;
   filter=0;
}

void DiagramWidget::setFunction(const hgz::Function<double,double>& func)
{
   pFunction.reset(new hgz::Function<double,double>(func));
}





// Histo2dWidget
/**
 * main constructor
 */
Histo2dWidget::Histo2dWidget(QWidget * parent, int xbins, int ybins)
   :  RenderArea(parent), fXBins(xbins), fYBins(ybins)
{
   if (parent==NULL) {
      this->setMinimumWidth(300);
      this->setMinimumHeight(250);
   }
   //fMask = QBitmap(this->width(),this->height());
   fBounds = QRectF(0.,0.,1.,1.);
   fGridX = true;
   fGridY = true;
   fLabels = true;
   fNrXTicks = -1;
   fNrYTicks = -1;
   fData.clear();
   fHisto = new QHist2d(fXBins,0.,24.,fYBins,-30.,90.);
   fAutoScale=true;
   setWindowTitle(tr("2D-Map"));
   fMenuBar=NULL;
   fBgColor = Qt::black;
   fGridColor = Qt::white;
   fHistPalette = QHist2d::COLOR1;
   fHistGradation = QHist2d::LINEAR;
   fHistInvert = false;
   fInterpolateBins = false;
   fInterpolateIterations = 1;
   fShowPalette = true;
   fShowGrid = true;
   fPaletteWidth = max(this->width()/20, 20);
   fVerbose=0;

   fBinningDialog = new MultiIntInputDialog(this,2);
   fBinningDialog->setWindowTitle(tr("Binning"));
   fBinningDialog->setLabelText(0,"X Bins");
   fBinningDialog->setLabelText(1,"Y Bins");
   fBinningDialog->setValue(0, fXBins);
   fBinningDialog->setValue(1, fYBins);
   fBinningDialog->setMinimum(0, 1);
   fBinningDialog->setMinimum(1, 1);
   fBinningDialog->setMaximum(0, 10000);
   fBinningDialog->setMaximum(1, 10000);

   connect(fBinningDialog, SIGNAL(valuesChanged(QVector<int>)), this, SLOT(setBins(QVector<int>)));
   if (fVerbose) std::cout<<"created binning dialog"<<std::endl;

   fBoundsDialog = new MultiDoubleInputDialog(this,4);
   fBoundsDialog->setWindowTitle(tr("Boundaries"));
   fBoundsDialog->setLabelText(0,"X Min");
   fBoundsDialog->setLabelText(1,"X Max");
   fBoundsDialog->setLabelText(2,"Y Min");
   fBoundsDialog->setLabelText(3,"Y Max");
   fBoundsDialog->setMinimum(-1e+38);
   fBoundsDialog->setMaximum(+1e+38);
   fBoundsDialog->setValue(0, fBounds.left());
   fBoundsDialog->setValue(1, fBounds.right());
   fBoundsDialog->setValue(2, fBounds.top());
   fBoundsDialog->setValue(3, fBounds.bottom());

   connect(fBoundsDialog, SIGNAL(valuesChanged(QVector<double>)), this, SLOT(setBounds(QVector<double>)));
   if (fVerbose) std::cout<<"created bounds dialog"<<std::endl;

   QGridLayout* layout;
   if (parent==NULL) layout = new QGridLayout;
   if (parent==NULL) layout->setRowStretch(0,1);
   if (parent==NULL) setLayout(layout);

   createActions();
   createMenus();
   createStatusBar();

   if (parent==NULL) layout->setContentsMargins(0,fMenuBar->height(),0,0);
   if (parent==NULL) layout->setVerticalSpacing(0);
//   setLayout(layout);

   setMouseTracking(true);

   int w=this->width() - (fShowPalette ? fPaletteWidth : 0);
   int h=this->height();
   if (parent==NULL) h-=fMenuBar->height()+fStatusBar->height();
   fMask = QBitmap(w,h);
   fPixmap = QPixmap(w,h);
   if (fGridX || fGridY) createMask();
   if (fShowPalette) drawPalette();

//   this->setAttribute(Qt::WA_PaintOnScreen);
   // to nullify small flashes with movement of mouse
//   this->setAttribute(Qt::WA_NoSystemBackground);
}

Histo2dWidget::~Histo2dWidget()
{
   if (fHisto) delete fHisto;
   if (fBinningDialog) delete fBinningDialog;
   if (fBoundsDialog) delete fBoundsDialog;
}

void Histo2dWidget::paintEvent(QPaintEvent * event)
{
   //if (!fData.size()) return;
   QPainter painter(this);
   int w=this->width();
   int h=this->height();
   if (this->parent()==NULL) h-=fMenuBar->height()+fStatusBar->height();
   int ystart=0;
   if (fMenuBar) ystart=fMenuBar->height();
//   painter.drawPixmap(QRect(0,ystart,w,h-ystart),fPixmap);
   painter.fillRect(0,ystart,fPixmap.width(),fPixmap.height(),fGridColor);
   painter.drawPixmap(0,ystart,fPixmap);
   if (fShowPalette) painter.drawPixmap(this->width()-fPaletteWidth,ystart,fPaletteWidth,h,fPalettePixmap);
}

void Histo2dWidget::resizeEvent(QResizeEvent * event)
{
   //return;
   //   fPixmap.fill(Qt::black);
   fPaletteWidth = max(this->width()/20, 20);
   int w=this->width() - (fShowPalette ? fPaletteWidth : 0);
   int h=this->height();
   if (fMenuBar) h-=fMenuBar->height()+fStatusBar->height();
//   fPixmap = QPixmap(w,h);
   if (fInterpolateBins) fPixmap = fHisto->interpolateBins(fInterpolateIterations).getPixmap(fHistPalette,fHistGradation,fHistInvert).scaled(w,h);
   else fPixmap = fHisto->getPixmap(fHistPalette,fHistGradation,fHistInvert).scaled(w,h);

//   fPixmap.fill(Qt::black);

//   if (fMask) { delete fMask; fMask = NULL; }
   fMask = QBitmap(w,h);
//   fMask.fill(Qt::color1);
   if (fVerbose) std::cout<<" created mask: w="<<fMask.width()<<" h="<<fMask.height()<<std::endl;
   if (fGridX || fGridY) { createMask(); /*fPixmap.setMask(fMask);*/ }
   //if (fShowPalette) drawPalette();
   draw();
}

void Histo2dWidget::fillHisto()
{
   fHisto->clear();
   if (!fData.size()) return;
   for (int i=0; i<fData.size(); i++)
   {
      fHisto->fill(fData[i].x(),fData[i].y(),fData[i].z());
   }
}

void Histo2dWidget::mouseMoveEvent(QMouseEvent * event)
{
   if (fVerbose>1) std::cout<<"mouseMoveEvent occured"<<std::endl;
   if (fVerbose>1) std::cout<<"x="<<event->pos().x()<<",y="<<event->pos().y()<<std::endl;
   int i=event->pos().x();
   int j=event->pos().y();
   if (parent()==NULL) j-=fMenuBar->height()/*+fStatusBar->height()*/;
   if (fPixmap.rect().contains(QPoint(i,j)))
   {
      double x=(double)i/fPixmap.width()*fBounds.width()+fBounds.left();
      double y=(fPixmap.height()-(double)j)/fPixmap.height()*fBounds.height()+fBounds.top();
      double z=fHisto->GetMeanBinContent(fHisto->toBinX(x),fHisto->toBinY(y));
      fStatusBar->showMessage("x="+QString::number(x)+" y="+QString::number(y)+" value="+QString::number(z));
   }
}

/**
 * assign data to the instance
 */
void Histo2dWidget::setData(const QVector< QPoint3F > & data)
{
   if (fHisto) delete fHisto;
   fHisto = new QHist2d(fXBins,fBounds.left(),fBounds.right(),fYBins,fBounds.top(),fBounds.bottom());
   fHisto->setBgColor(fBgColor);
   fData=data;
   if (fAutoScale) autoscale(fAutoScale);
   else {
      //createMask();
      fillHisto();
		draw();
   }
}

void Histo2dWidget::drawPalette()
{
   fPalettePixmap = fHisto->getPalette(fHistPalette, fHistInvert);
}

/**
 * main draw function of widget. draws data on internal pixmap and sets grid mask
 */
void Histo2dWidget::draw()
{
   //if (!fData.size()) return;
   if (fVerbose>1) std::cout<<"begin of *Histo2dWidget::draw()*"<<std::endl;

   //if (fShowGrid) fPixmap.fill(fGridColor);
  /*
   int w=this->width();
   int h=this->height();
   if (fMenuBar) h-=fMenuBar->height();
   fPixmap = fHisto->getPixmap().scaled(w,h);
  */
   if (fVerbose) cout<<"min entry="<<fHisto->getMinMean()<<endl;
   if (fVerbose) cout<<"max entry="<<fHisto->getMaxMean()<<endl;
   if (fInterpolateBins) fPixmap = fHisto->interpolateBins(fInterpolateIterations).getPixmap(fHistPalette,fHistGradation,fHistInvert).scaled(fPixmap.width(),fPixmap.height());
   else fPixmap = fHisto->getPixmap(fHistPalette,fHistGradation,fHistInvert).scaled(fPixmap.width(),fPixmap.height());

   if (fVerbose>1) std::cout<<"histo filled"<<std::endl;

   if ((fGridX || fGridY)) {
//      createMask();
      fPixmap.setMask(fMask);
   }

   update();
   if (fVerbose>1) std::cout<<"end of *Histo2dWidget::draw()*"<<std::endl;

   return;
}

void Histo2dWidget::fill(double x, double y, double value)
{
   fHisto->fill(x,y,value);
   fData.push_back(QPoint3F(x,y,value));
   if (fAutoScale) autoscale(fAutoScale);
   else { createMask(); draw(); }
}

/**
 * set visible window bounds
 * @param rect ( min(x), min(y), range(x), range(y) )
 */
void Histo2dWidget::setBounds(const QRectF & rect)
{
   fBounds = rect;
   adjustBounds();

   fBoundsDialog->setValue(0, fBounds.left());
   fBoundsDialog->setValue(1, fBounds.right());
   fBoundsDialog->setValue(2, fBounds.top());
   fBoundsDialog->setValue(3, fBounds.bottom());

   if (fHisto) delete fHisto;
   fHisto = new QHist2d(fXBins,fBounds.left(),fBounds.right(),fYBins,fBounds.top(),fBounds.bottom());
   if (fVerbose>1) std::cout<<"set bounds to ("<<fBounds.left()<<","<<fBounds.top()<<","<<fBounds.right()<<","<<fBounds.bottom()<<")"<<std::endl;
   fHisto->setBgColor(fBgColor);
   if (fGridX || fGridY) createMask();
   fillHisto();
   draw();
}

/**
 * set number of bins on x-axis
 * @param bins number of bins
 */
void Histo2dWidget::setBinsX(int bins)
{
   fXBins=bins;
   setBounds(fBounds);
}

/**
 * set number of bins on y-axis
 * @param bins number of bins
 */
void Histo2dWidget::setBinsY(int bins)
{
   fYBins=bins;
   setBounds(fBounds);
}

/**
 * set number of bins on both axis
 * @param xbins number of bins in x-direction
 * @param ybins number of bins in y-direction
 */
void Histo2dWidget::setBins(int xbins, int ybins)
{
   fXBins=xbins;
   fYBins=ybins;
   setBounds(fBounds);
}

/**
 * set left boundary of displayed area
 * @param xmin value of left boundary
 */
void Histo2dWidget::setXMin(double xmin)
{
   fBounds.setLeft(xmin);
   setBounds(fBounds);
}

/**
 * set right boundary of displayed area
 * @param xmax value of right boundary
 */
void Histo2dWidget::setXMax(double xmax)
{
   fBounds.setRight(xmax);
   setBounds(fBounds);
}

/**
 * set top boundary of displayed area
 * @param ymin value of top boundary
 */
void Histo2dWidget::setYMin(double ymin)
{
   fBounds.setTop(ymin);
   setBounds(fBounds);
}

/**
 * set bottom boundary of displayed area
 * @param ymax value of bottom boundary
 */
void Histo2dWidget::setYMax(double ymax)
{
   fBounds.setBottom(ymax);
   setBounds(fBounds);
}

void Histo2dWidget::contextMenuEvent(QContextMenuEvent * event)
{
   if (fVerbose) std::cout<<"context menu event"<<std::endl;


   //fBinningDialog->show();

/*
   QWidgetAction* w_act=new QWidgetAction(this);
//   QSpinBox* spinbox = new QSpinBox(this);
   QSlider* slider = new QSlider(this);
   slider->setOrientation(Qt::Horizontal);
   w_act->setDefaultWidget(slider);
   menu.addAction(w_act);
*/
   contextMenu->exec(event->globalPos());
}

/**
 * automatically scale boundaries of displayed area according to the range which is spanned by the data
 * @param autoscale switch on/off autoscaling
 */
void Histo2dWidget::autoscale(bool autoscale)
{
   fAutoScale=autoscale;
   if (!fData.size()) return;
	if (!fAutoScale) return;

   double x_min=1e+38;
   double x_max=-1e+38;
   double y_min=1e+38;
   double y_max=-1e+38;

   // Min/Max x+y
   for (int i=0; i<fData.size(); i++)
   {
      double x=fData[i].x();
      double y=fData[i].y();
      if (x<x_min) x_min=x;
      if (x>x_max) x_max=x;
      if (y<y_min) y_min=y;
      if (y>y_max) y_max=y;
   }
   if (fVerbose) std::cout<<"autoscale"<<std::endl;
   if (fVerbose) std::cout<<" xmin="<<x_min<<" xmax="<<x_max<<" ymin="<<y_min<<" ymax="<<y_max<<std::endl;
   setBounds(QRectF(x_min-0.05*(x_max-x_min),
                    y_min-0.05*(y_max-y_min),
                    1.1*(x_max-x_min),
                    1.1*(y_max-y_min)));
}

void Histo2dWidget::adjustBounds()
{
   if (fBounds.left()==fBounds.right()) {
      if (fBounds.left()==0.) {
         fBounds.setLeft(-0.1);
         fBounds.setRight(0.1);
      } else {
         fBounds.setLeft(fBounds.left()-0.1*fabs(fBounds.left()));
         fBounds.setRight(fBounds.right()+0.1*fabs(fBounds.right()));
      }
      if (fVerbose>1) {
         cout<<"modified x-bounds to ("<<fBounds.left()<<","<<fBounds.right()<<")"<<endl;
      }
   }

   if (fBounds.top()==fBounds.bottom()) {
      if (fBounds.top()==0.) {
         fBounds.setTop(-0.1);
         fBounds.setBottom(0.1);
      } else {
         fBounds.setTop(fBounds.top()-0.1*fabs(fBounds.top()));
         fBounds.setBottom(fBounds.bottom()+0.1*fabs(fBounds.bottom()));
      }
      if (fVerbose>1) {
         cout<<"modified y-bounds to ("<<fBounds.top()<<","<<fBounds.bottom()<<")"<<endl;
      }
   }
}



/**
 * set number of bins on both axis, overloaded function provided for convenience
 * @param bins vector with two entries which hold the number of bins in x-direction and y-direction, respectively
 */
void Histo2dWidget::setBins(const QVector<int> & bins)
{
   fXBins=bins[0];
   fYBins=bins[1];
   setBounds(fBounds);
}

void Histo2dWidget::closeEvent(QCloseEvent * event)
{
   fBinningDialog->close();
   fBoundsDialog->close();
}

/**
 * clear data
 */
void Histo2dWidget::clear()
{
   fData.clear();
   fHisto->clear();
   setBounds(fBounds);
   draw();
}

void Histo2dWidget::saveImage(const QString & filename)
{
   int x=fPixmap.width() + (fShowPalette ? fPaletteWidth : 0);
   int y=fPixmap.height();
   QPixmap pm(x,y);
   pm.fill(fGridColor);
   QPainter painter(&pm);
   painter.drawPixmap(0,0,fPixmap);
   if (fShowPalette) painter.drawPixmap(x-fPaletteWidth,0,fPaletteWidth,y,fPalettePixmap);
   pm.save(filename,0,100);
}

void Histo2dWidget::saveImage()
{
   QString* filter=new QString("*.png");
   QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                            ".", tr("Images (*.png *.xpm *.jpg)"), filter);
   if (filename.size()) saveImage(filename);
   delete filter;
   filter=0;
}

void Histo2dWidget::exportData()
{
   QString* filter=new QString("*.fits");
   QString filename = QFileDialog::getSaveFileName(this, tr("Export Data"),
                            ".", tr("Text Files (*.asc);;FITS Files (*.fits *.fit)"), filter);
   if (filename.size()) {
      if (filter->contains("fit")) {
         // save fits file
         Array2D<double> arr(fHisto->nrBinsX(),fHisto->nrBinsY());
         for (int j=0; j<fHisto->nrBinsY(); j++)
            for (int i=0; i<fHisto->nrBinsX(); i++) {
               if (fHisto->GetBinMultiplicity(i,j)) {
                  arr[i][j]=fHisto->GetMeanBinContent(i,j);
               } else {
//                  arr[i][j]=DBL_MIN;
                  arr[i][j]=NAN;
               }
            }
         
         // create the keys for scaling of positions
         
         std::vector<keyword> keys;
	 double xrange=fBounds.right()-fBounds.left();
	 double yrange=-fBounds.top()+fBounds.bottom();
	 double xstep=xrange/fHisto->nrBinsX();
	 double ystep=yrange/fHisto->nrBinsY();
	 double xref=fBounds.left();
	 double yref=fBounds.top();
	 keys.push_back(keyword("CRVAL1",xref,"RA--     REF POINT VALUE (DEG)"));
	 keys.push_back(keyword("CRPIX1",1.0,"RA--     REF POINT PIXEL LOCATION"));
	 keys.push_back(keyword("CDELT1",xstep,"RA--     INCREMENT ALONG AXIS"));
	 keys.push_back(keyword("CTYPE1","RA--    ","RA--     PROJECTION TYPE"));
	 keys.push_back(keyword("CRVAL2",yref,"DEC-     REF POINT VALUE (DEG)"));
	 keys.push_back(keyword("CRPIX2",1.0,"DEC-     REF POINT PIXEL LOCATION"));
	 keys.push_back(keyword("CDELT2",ystep,"DEC-     INCREMENT ALONG AXIS"));
	 keys.push_back(keyword("CTYPE2","DEC-    ","DEC-     PROJECTION TYPE"));
	 
	 //CRVAL1  =                    0          / RA--     REF POINT VALUE (DEG)
	 //CRPIX1  =                  721          / RA--     REF POINT PIXEL LOCATION
	 //CDELT1  =                -0.25          / RA--     INCREMENT ALONG AXIS
	 //CTYPE1  = 'RA--    '                    / RA--     PROJECTION TYPE
	 
	 
         Fits::write(filename.toStdString(), arr, false, true, keys);
      } else {
         /// TODO: save ascii table
	 // still to implement
      }
   }
   delete filter;
   filter=0;
}

double Histo2dWidget::bcdExtend(double number)
{
   if (number==0.) return number;
   double expo = pow(10.,floor(log10(fabs(number))));
   if (fVerbose>1) std::cout<<" expo = "<<expo<<std::endl;
   double base = fabs(number/expo);
   if (fVerbose>1) std::cout<<" base = "<<base<<std::endl;
   base = std::floor(base+0.5);
   int iBase = (int)base;
   if (fVerbose>1) std::cout<<" iBase = "<<iBase<<std::endl;
   switch (iBase)
   {
      case 1 : base = 2.;
               break;
      case 2 :
      case 3:
      case 4:  base = 5.;
               break;
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:  base = 10.;
               break;
      case 10: base = 20.; // eigentlich unnötig, oder?
               break;
      default: return number;
   }
   if (fVerbose>1) std::cout<<"extended to base = "<<base<<std::endl;
   double result = fabs(base*expo);
   if (fVerbose>1) std::cout<<"extended to "<<result<<std::endl;
   return (result > 0) ? result : -result;
}

double Histo2dWidget::bcdReduce(double number)
{
   if (number==0.) return number;
   double expo = pow(10.,floor(log10(fabs(number))));
   double base = fabs(number/expo);
   if (fVerbose>1) std::cout<<" base = "<<base<<std::endl;
   base = std::floor(base+0.5);
   int iBase = (int)base;
   switch (iBase)
   {
      case 1 : base = 0.5;
               break;
      case 2 : base = 1.;
               break;
      case 3:
      case 4:
      case 5:  base = 2.;
               break;
      case 6:
      case 7:
      case 8:
      case 9:
      case 10: base = 5.; // eigentlich unnötig, oder?
               break;
      default: return number;
   }
   double result = fabs(base*expo);
   return (result > 0) ? result : -result;
}

void Histo2dWidget::createMask()
{
   if (!fMask) return;
//   fMask.clear();
   fMask.fill(Qt::color1);

   QPainter painter(&fMask);
   QPen pen(Qt::color0);
   pen.setStyle(Qt::DotLine);

   painter.setPen(pen);
   QFont sansFont("Helvetica", 9, QFont::Bold);
//   QFont sansFont("Helvetica [Cronyx]", 8);
   painter.setFont(sansFont);

   // vertical axis
   if (fGridY && fBounds.top()!=fBounds.bottom()) {
      int maxTicks = std::max(fMask.height()/50,9);
      if (fVerbose>1) std::cout<<"max. nr. y-ticks : "<<maxTicks<<std::endl;
      //int maxTicks = 8;
      int minTicks = 4;

      double interval = pow(10.,std::floor(log10(fabs(fBounds.height())))-1.);
      if (fVerbose>1) std::cout<<"y-tick interval(0) : "<<interval<<std::endl;
      int nrTicks = fabs(fBounds.height())/interval;
      if (fVerbose>1) std::cout<<"nr. of y-ticks(0) : "<<nrTicks<<std::endl;
      while (nrTicks < minTicks || nrTicks > maxTicks) {
         if (nrTicks < minTicks) interval = bcdReduce(interval);
         else if (nrTicks > maxTicks) interval = bcdExtend(interval);
         else break;
         nrTicks = fabs(fBounds.height())/interval;
         if (fVerbose>1) std::cout<<"y-tick interval : "<<interval<<std::endl;
      }
      if (fVerbose>1) std::cout<<"y-tick interval : "<<interval<<std::endl;
      if (fVerbose>1) std::cout<<"nr of y-ticks : "<<nrTicks<<std::endl;
      double tickLevel = interval*std::floor((double)fBounds.top()/interval+0.5);
      if (fVerbose>1) std::cout<<"first y-tick : "<<tickLevel<<std::endl;

      for (int i=0; i<nrTicks; i++)
      {
         int precision = -(std::floor(log10(interval)));
         //   if (precision<0) precision = 0;
         if (fVerbose>1) std::cout<<"x precision = "<<precision<<std::endl;
         int y=fMask.height()*(1.-(tickLevel-fBounds.top())/(fBounds.height()));
         painter.drawLine(QPoint(0,y),QPoint(fMask.width()-1,y));

//         painter.drawText(QPointF(this->width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel,'g',(precision>0)?precision:0));

         if (fLabels) {
            if (fabs(tickLevel) < 1000.) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel,'f',(precision>0)?precision:0));
            } else if (fabs(tickLevel) < 1e+6) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1000.,'f',(precision+3>0)?precision+3:0)+"k");
            } else if (fabs(tickLevel) < 1e+9) {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1e+6,'f',(precision+6>0)?precision+6:0)+"M");
            } else {
               painter.drawText(QPointF(fMask.width()-(abs(log10(interval))+3)*9,y-1),QString::number(tickLevel/1e+9,'f',(precision+9>0)?precision+9:0)+"G");
            }
         }

         tickLevel+=sgn(fBounds.height())*interval;
      }
   }

   if (fGridX && fBounds.left()!=fBounds.right()) {
      // horizontal axis
      int maxTicks = std::max(fMask.width()/50,10);
      if (fVerbose>1) std::cout<<"max. nr. x-ticks : "<<maxTicks<<std::endl;
      int minTicks = 4;

      double interval = pow(10.,std::floor(log10(fabs(fBounds.width())))-1.);
      if (fVerbose>1) std::cout<<"x-tick interval(0) : "<<interval<<std::endl;
      int nrTicks = fabs(fBounds.width())/interval;
      if (fVerbose>1) std::cout<<"nr. of x-ticks(0) : "<<nrTicks<<std::endl;
      while (nrTicks < minTicks || nrTicks > maxTicks) {
         if (nrTicks < minTicks) interval = bcdReduce(interval);
         else if (nrTicks > maxTicks) interval = bcdExtend(interval);
         else break;
         nrTicks = fabs(fBounds.width())/interval;
         if (fVerbose>1) std::cout<<"x-tick interval : "<<interval<<std::endl;
      }
      if (fVerbose>1) std::cout<<"x-tick interval : "<<interval<<std::endl;
      if (fVerbose>1) std::cout<<"nr of x-ticks : "<<nrTicks<<std::endl;
      double tickLevel = interval*std::floor((double)fBounds.left()/interval+0.5);
      if (fVerbose>1) std::cout<<"first x-tick : "<<tickLevel<<std::endl;

      int precision = -(std::floor(log10(interval)));
   //   if (precision<0) precision = 0;
      if (fVerbose>1) std::cout<<"precision = "<<precision<<std::endl;
      for (int i=0; i<nrTicks; i++)
      {
         int x=fMask.width()*((tickLevel-fBounds.left())/fBounds.width());
         painter.drawLine(QPoint(x,0),QPoint(x,fMask.height()-1));

         if (fLabels) {
            if (fabs(tickLevel) < 1000.) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel,'f',(precision>0)?precision:0));
            } else if (fabs(tickLevel) < 1e+6) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1000.,'f',(precision+3>0)?precision+3:0)+"k");
            } else if (fabs(tickLevel) < 1e+9) {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1e+6,'f',(precision+6>0)?precision+6:0)+"M");
            } else {
               painter.drawText(QPointF(x,0.1*fMask.height()),QString::number(tickLevel/1e+9,'f',(precision+9>0)?precision+9:0)+"G");
            }
         }
         tickLevel+=sgn(fBounds.width())*interval;
      }
   }
   //fPixmap.setMask(fMask);
}

void Histo2dWidget::createActions()
{
      saveAct = new QAction(QIcon(":/filesave.xpm"), tr("&Save"), this);
      saveAct->setShortcut(tr("Ctrl+S"));
      saveAct->setStatusTip(tr("Save Image to disk"));
      connect(saveAct, SIGNAL(triggered()), this, SLOT(saveImage()));

      exportAct = new QAction(tr("&Export data"), this);
      exportAct->setStatusTip(tr("export data to ASCII list"));
      connect(exportAct, SIGNAL(triggered()), this, SLOT(exportData()));

      exitAct = new QAction(tr("C&lose"), this);
      exitAct->setShortcut(tr("Ctrl+Q"));
      exitAct->setStatusTip(tr("Close the window"));
      connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

      aboutAct = new QAction(tr("&About"), this);
      aboutAct->setStatusTip(tr("Show the application's About box"));
      connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

      aboutQtAct = new QAction(tr("About &Qt"), this);
      aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
      connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

      interpolAct = new QAction(tr("&Interpolate Missing Bins"), this);
      interpolAct->setStatusTip(tr("Interpolate Missing Bins"));
      interpolAct->setCheckable(true);
      interpolAct->setChecked(fInterpolateBins);
      connect(interpolAct, SIGNAL(toggled(bool)), this, SLOT(interpolateBins(bool)));

      interpolIterationsAct = new QAction(tr("&Interpolation Multiplicity"), this);
      interpolIterationsAct->setStatusTip(tr("Number of neighboring bins used for interpolation"));
      connect(interpolIterationsAct, SIGNAL(triggered()), this, SLOT(chooseInterpolIterations()));

      autoscaleAct=new QAction("&Autoscale",this);
      autoscaleAct->setStatusTip(tr("automatically scale the histograms boundaries"));
      autoscaleAct->setCheckable(true);
      autoscaleAct->setChecked(fAutoScale);
      connect(autoscaleAct, SIGNAL(toggled(bool)), this, SLOT(autoscale(bool)));

      binningAct=new QAction("&Binning",this);
      connect(binningAct, SIGNAL(triggered()), fBinningDialog, SLOT(show()));

      boundaryAct=new QAction("B&oundaries",this);
      connect(boundaryAct, SIGNAL(triggered()), fBoundsDialog, SLOT(show()));

      greyscaleAct=new QAction("&Greyscale",this);
      greyscaleAct->setCheckable(true);
      connect(greyscaleAct, SIGNAL(triggered()), this, SLOT(setPalette()));

      colorAct=new QAction("&False Color #1",this);
      colorAct->setCheckable(true);
      connect(colorAct, SIGNAL(triggered()), this, SLOT(setPalette()));

      monochrome1Act=new QAction("&Monochrome #1",this);
      monochrome1Act->setCheckable(true);
      connect(monochrome1Act, SIGNAL(triggered()), this, SLOT(setPalette()));

      monochrome2Act=new QAction("&Monochrome #2",this);
      monochrome2Act->setCheckable(true);
      connect(monochrome2Act, SIGNAL(triggered()), this, SLOT(setPalette()));

      monochrome3Act=new QAction("&Monochrome #3",this);
      monochrome3Act->setCheckable(true);
      connect(monochrome3Act, SIGNAL(triggered()), this, SLOT(setPalette()));

      monochrome4Act=new QAction("&Monochrome #4",this);
      monochrome4Act->setCheckable(true);
      connect(monochrome4Act, SIGNAL(triggered()), this, SLOT(setPalette()));

      monochrome5Act=new QAction("&Monochrome #5",this);
      monochrome5Act->setCheckable(true);
      connect(monochrome5Act, SIGNAL(triggered()), this, SLOT(setPalette()));

      paletteGroup = new QActionGroup(this);
      paletteGroup->addAction(greyscaleAct);
      paletteGroup->addAction(colorAct);
      paletteGroup->addAction(monochrome1Act);
      paletteGroup->addAction(monochrome2Act);
      paletteGroup->addAction(monochrome3Act);
      paletteGroup->addAction(monochrome4Act);
      paletteGroup->addAction(monochrome5Act);
      colorAct->setChecked(true);
//      connect(paletteGroup, SIGNAL(triggered()), this, SLOT(setPalette()));

      linearAct=new QAction("&Linear",this);
      linearAct->setCheckable(true);
      connect(linearAct, SIGNAL(triggered()), this, SLOT(setGradation()));

      logAct=new QAction("&Logarithmic",this);
      logAct->setCheckable(true);
      connect(logAct, SIGNAL(triggered()), this, SLOT(setGradation()));

      gradationGroup = new QActionGroup(this);
      gradationGroup->addAction(linearAct);
      gradationGroup->addAction(logAct);
      linearAct->setChecked(true);

      invertAct=new QAction("&Invert Colorscale",this);
      invertAct->setStatusTip(tr("invert the active color palette"));
      invertAct->setCheckable(true);
      invertAct->setChecked(fHistInvert);
      connect(invertAct, SIGNAL(toggled(bool)), this, SLOT(invertPalette(bool)));

      bgColAct = new QAction(tr("&Background Color"), this);
      bgColAct->setStatusTip(tr("Set the Background Color"));
      connect(bgColAct, SIGNAL(triggered()), this, SLOT(chooseBgColor()));

      gridColAct = new QAction(tr("&Grid Color"), this);
      gridColAct->setStatusTip(tr("Set the Grid Color"));
      connect(gridColAct, SIGNAL(triggered()), this, SLOT(chooseGridColor()));

      showGridAct=new QAction("&Grid",this);
      showGridAct->setStatusTip(tr("superimposes a grid to the histogram"));
      showGridAct->setCheckable(true);
      showGridAct->setChecked(fShowGrid);
      connect(showGridAct, SIGNAL(toggled(bool)), this, SLOT(showGrid(bool)));

      showPaletteAct=new QAction("&Show Palette",this);
      showPaletteAct->setStatusTip(tr("shows the actual palette beside the histogram"));
      showPaletteAct->setCheckable(true);
      showPaletteAct->setChecked(fShowPalette);
      connect(showPaletteAct, SIGNAL(toggled(bool)), this, SLOT(showPalette(bool)));

}

void Histo2dWidget::interpolateBins(bool interpol)
{
   if (fVerbose) cout<<"Histo2dWidget::interpolateBins()"<<endl;
   fInterpolateBins=interpol;
   draw();
   return;

   fHisto->interpolateBins();
   fPixmap = fHisto->getPixmap(fHistPalette,fHistGradation,fHistInvert).scaled(fPixmap.width(),fPixmap.height());
   if ((fGridX || fGridY) && fMask.rect() == fPixmap.rect()) {
//      createMask();
      fPixmap.setMask(fMask);
   }
   update();
}

void Histo2dWidget::chooseInterpolIterations()
{
  bool ok;
  int i = QInputDialog::getInt(this, tr("Interpolation Iteration Number"),
                                  tr("Nr. of neighboring bins for interpolation:"), fInterpolateIterations, 0, 100, 1, &ok);
  if (ok)
     fInterpolateIterations = i;
  draw();
}

void Histo2dWidget::chooseBgColor()
{
   if (fVerbose) cout<<"Histo2dWidget::chooseBgColor()"<<endl;
   QColor col=QColorDialog::getColor(fBgColor,this);
   if (col.isValid()) { fBgColor=col; fHisto->setBgColor(fBgColor); draw(); }
}

void Histo2dWidget::chooseGridColor()
{
   if (fVerbose) cout<<"Histo2dWidget::chooseGridColor()"<<endl;
   QColor col=QColorDialog::getColor(fGridColor,this);
   if (col.isValid()) { fGridColor=col; update(); }
}

void Histo2dWidget::invertPalette(bool invert)
{
  if (fVerbose>2) cout<<"Histo2dWidget::invertPalette(bool)"<<endl;
  fHistInvert = invert;
  draw();
  if (fShowPalette) drawPalette();
}


void Histo2dWidget::setPalette()
{
  if (fVerbose>2) cout<<"Histo2dWidget::setPalette()"<<endl;
  if (greyscaleAct->isChecked()) {
      fHistPalette=QHist2d::GREYSCALE;
      if (fVerbose>1) cout<<"greyscale"<<endl;
   } else
   if (colorAct->isChecked()) {
      fHistPalette=QHist2d::COLOR1;
      if (fVerbose>1) cout<<"color1"<<endl;
   } else
   if (monochrome1Act->isChecked()) {
      fHistPalette=QHist2d::MONO_YELLOW;
      if (fVerbose>1) cout<<"mono1"<<endl;
   } else
   if (monochrome2Act->isChecked()) {
      fHistPalette=QHist2d::MONO_GREEN;
      if (fVerbose>1) cout<<"mono2"<<endl;
   } else
   if (monochrome3Act->isChecked()) {
      fHistPalette=QHist2d::MONO_BLUE;
      if (fVerbose>1) cout<<"mono3"<<endl;
   } else
   if (monochrome4Act->isChecked()) {
      fHistPalette=QHist2d::MONO_RED;
      if (fVerbose>1) cout<<"mono4"<<endl;
   } else
   if (monochrome5Act->isChecked()) {
      fHistPalette=QHist2d::MONO_PURPLE;
      if (fVerbose>1) cout<<"mono5"<<endl;
   } else
   {
      fHistPalette=QHist2d::COLOR1;
   }
   if (fShowPalette) drawPalette();
   draw();
}

void Histo2dWidget::setGradation()
{
   if (fVerbose>2) cout<<"Histo2dWidget::setGradation()"<<endl;
   if (linearAct->isChecked()) {
      fHistGradation=QHist2d::LINEAR;
      if (fVerbose>1) cout<<"linear"<<endl;
   } else
   if (logAct->isChecked()) {
      fHistGradation=QHist2d::LOG;
      if (fVerbose>1) cout<<"log"<<endl;
   } else
   {
      fHistGradation=QHist2d::LINEAR;
   }
   draw();
}

void Histo2dWidget::createMenus()
{
   contextMenu = new QMenu("2D-Map",this);
   contextMenu->addAction(autoscaleAct);
   contextMenu->addSeparator();
   contextMenu->addAction(binningAct);
   contextMenu->addAction(boundaryAct);
   contextMenu->addAction(showGridAct);
   contextMenu->addSeparator();
   contextMenu->addAction(saveAct);
   contextMenu->addAction(exportAct);
   contextMenu->addSeparator();

   contextMenu->addAction(invertAct);
   contextMenu->addAction(showPaletteAct);

   QMenu* paletteMenu = new QMenu("Palette",this);
   paletteMenu->addAction(greyscaleAct);
   paletteMenu->addAction(colorAct);
   paletteMenu->addAction(monochrome1Act);
   paletteMenu->addAction(monochrome2Act);
   paletteMenu->addAction(monochrome3Act);
   paletteMenu->addAction(monochrome4Act);
   paletteMenu->addAction(monochrome5Act);
   contextMenu->addMenu(paletteMenu);

   QMenu* gradationMenu = new QMenu("Gradation",this);
   gradationMenu->addAction(linearAct);
   gradationMenu->addAction(logAct);
   contextMenu->addMenu(gradationMenu);

   contextMenu->addAction(bgColAct);
   contextMenu->addAction(gridColAct);
   contextMenu->addSeparator();
   contextMenu->addAction(interpolAct);
   contextMenu->addAction(interpolIterationsAct);

   if (parent()==NULL) {
      fMenuBar = new QMenuBar(this);
   //   menuBar->setFrameStyle(QFrame::Panel | QFrame::Raised);
      //menuBar->setStyle(new QMotifStyle());
   //   layout->addWidget(menuBar,0,0);
      //layout->setRowStretch(0,0);

      fileMenu = fMenuBar->addMenu(tr("&File"));
      fileMenu->addAction(saveAct);
      fileMenu->addAction(exportAct);
      fileMenu->addSeparator();
      fileMenu->addAction(exitAct);

      editMenu = fMenuBar->addMenu(tr("&Edit"));
      editMenu->addAction(autoscaleAct);
      editMenu->addAction(binningAct);
      editMenu->addAction(boundaryAct);
      editMenu->addAction(showGridAct);
      editMenu->addSeparator();
      editMenu->addAction(invertAct);
      editMenu->addAction(showPaletteAct);
      editMenu->addMenu(paletteMenu);
      editMenu->addMenu(gradationMenu);
      editMenu->addAction(bgColAct);
      editMenu->addSeparator();
      editMenu->addAction(interpolAct);
      editMenu->addAction(interpolIterationsAct);
      
      fMenuBar->addSeparator();

      helpMenu = fMenuBar->addMenu(tr("&Help"));
      helpMenu->addAction(aboutAct);
      helpMenu->addAction(aboutQtAct);
   }

}

void Histo2dWidget::createStatusBar()
{
   if (parent()) return;
   fStatusBar = new QStatusBar(this);
   //statusBar->setMaximumHeight(16);
   //statusBar->setMinimumHeight(16);
   ((QGridLayout*)layout())->addWidget(fStatusBar,1,0);
   ((QGridLayout*)layout())->setRowStretch(1,0);
   fStatusBar->sizePolicy().setVerticalPolicy(QSizePolicy::Fixed);
//   fStatusBar->showMessage(tr("Ready"));
   //statusBar->addWidget(new QFrame(this));
   //statusBar->addWidget(new QProgressBar);
   fStatusBar->setSizeGripEnabled(true);
}

void Histo2dWidget::showPalette(bool showPalette)
{
   fShowPalette = showPalette;
   if (fShowPalette) drawPalette();
   QResizeEvent* ev;
   this->resizeEvent(ev);
   update();
}

void Histo2dWidget::showGrid(bool showGrid)
{
   fShowGrid = showGrid;
   fGridX=fGridY=showGrid;
   if (fShowGrid) createMask();
   draw();
}
