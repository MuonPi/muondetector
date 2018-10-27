/***************************************************************************
 *   Copyright (C) 2007 by Hans-Georg Zaunick   *
 *   hg.zaunick@physik.tu-dresden.de   *
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

#include <QImage>
#include <QPainter>
#include "QHistogram.h"
#include <cmath>
#include <iostream>

QHist2d::QHist2d() : _nrBins(0), _nrBinsX(0), _nrBinsY(0), _nrEntries(0),
                     _startX(0.), _endX(0.), _startY(0.), _endY(0.), _data(0),
                     _underflowX(0), _overflowX(0), _underflowY(0), _overflowY(0), fBgColor(Qt::black)
{
}

QHist2d::QHist2d(const QHist2d& a_hist)
   : _nrBins(a_hist.nrBinsX()*a_hist.nrBinsY()), _nrBinsX(a_hist.nrBinsX()), _nrBinsY(a_hist.nrBinsY()), _nrEntries(a_hist._nrEntries),
     _startX(a_hist.startX()), _endX(a_hist.endX()), _startY(a_hist.startY()), _endY(a_hist.endY()),
     _underflowX(a_hist.underflowX()), _overflowX(a_hist.overflowX()), _underflowY(a_hist.underflowY()), _overflowY(a_hist.overflowY()),
     fBgColor(a_hist.bgColor())
{
   _data = new std::vector<double>[_nrBins];
   for (int i=0; i<_nrBins; i++) {
      _data[i]=a_hist._data[i];
   }
}

QHist2d::~ QHist2d()
{
   if (_data) delete[] _data;
}

QHist2d::QHist2d(int nrBinsX, double startX, double endX, int nrBinsY,
                 double startY, double endY)
   : _nrBins(nrBinsX*nrBinsY), _nrBinsX(nrBinsX), _nrBinsY(nrBinsY),
     _nrEntries(0), _startX(startX), _endX(endX), _startY(startY), _endY(endY),
     _underflowX(0), _overflowX(0), _underflowY(0), _overflowY(0), fBgColor(Qt::black)
{
//   _data = new double[_nrBins];
   _data = new std::vector<double>[_nrBins];
}

QHist2d& QHist2d::operator=(const QHist2d& a_hist)
{
   if (this == &a_hist) return *this;

   _nrBins=a_hist.nrBinsX()*a_hist.nrBinsY();
   _nrBinsX=a_hist.nrBinsX();
   _nrBinsY=a_hist.nrBinsY();
   _nrEntries=a_hist._nrEntries;
   _startX=a_hist.startX();
   _endX=a_hist.endX();
   _startY=a_hist.startY();
   _endY=a_hist.endY();
   _underflowX=a_hist.underflowX();
   _overflowX=a_hist.overflowX();
   _underflowY=a_hist.underflowY();
   _overflowY=a_hist.overflowY();
   fBgColor=a_hist.bgColor();

   if (_data) delete[] _data;
   _data = new std::vector<double>[_nrBins];
   for (int i=0; i<_nrBins; i++) {
      _data[i]=a_hist._data[i];
   }

   return *this;
}

void QHist2d::fill(double valueX, double valueY, const double mult)
{
   if (!_data) return;
   int xbin=toBinX(valueX);
   int ybin=toBinY(valueY);
   if (xbin<0) { _underflowX+=mult; return; }
   else if (xbin>=_nrBinsX) { _overflowX+=mult; return; }
   else if (ybin<0) { _underflowY+=mult; return; }
   else if (ybin>=_nrBinsY) { _overflowY+=mult; return; }
   else {
      _data[ybin*_nrBinsX+xbin].push_back(mult);
//      _data[ybin*_nrBinsX+xbin] += mult;
//      _nrEntries += mult;
      _nrEntries++;
   }
}

void QHist2d::fillBin(int xbin, int ybin, const double mult)
{
   if (!_data) return;
   if (xbin<0) { _underflowX+=mult; return; }
   else if (xbin>=_nrBinsX) { _overflowX+=mult; return; }
   else if (ybin<0) { _underflowY+=mult; return; }
   else if (ybin>=_nrBinsY) { _overflowY+=mult; return; }
   else {
      _data[ybin*_nrBinsX+xbin].push_back(mult);
//      _data[ybin*_nrBinsX+xbin] += mult;
//      _nrEntries += mult;
      _nrEntries++;
   }
}

void QHist2d::fillGauss(double valueX, double valueY, double mult, double sigma)
{

}

QImage QHist2d::getImage(HISTO_PALETTE palette, HISTO_GRADATION gradation, bool invert)
{
   QImage* image = new QImage(_nrBinsX,_nrBinsY,QImage::Format_RGB32);

   image->fill(fBgColor.rgb());
//   image->fill(Qt::black);

   if (!_nrEntries) return *image;

   QPainter p(image);
   QPen pen;

   double min=getMinMean();
   double max=getMaxMean();
   if (max==min) {
      if (max<0.) max=0.;
      else min=0.;
   }

   double range=max-min;

   for (int y=0; y<_nrBinsY; y++)
      for (int x=0; x<_nrBinsX; x++)
   {
      if (!GetBinMultiplicity(x,y)) {
         pen.setColor(fBgColor);
         p.setPen(pen);
         p.drawPoint(x,_nrBinsY-y-1);
         continue;
      }
      double val=GetMeanBinContent(x,y);
//      double val=GetBinMultiplicity(x,y);
      val=(val-min)/range;

      if (gradation==LOG) val=log10(val*range+1.)/log10(range+1.);
      if (invert) val=1.-val;
      QColor col;
      switch (palette) {
        case GREYSCALE:
          col=getGreyScale(val);
          break;
        case COLOR1:
          col=getFalseColor(val);
          break;
        case MONO_YELLOW:
          col=getMonochrome(val, Qt::yellow);
          break;
        case MONO_GREEN:
          col=getMonochrome(val, Qt::green);
          break;
        case MONO_BLUE:
          col=getMonochrome(val, Qt::blue);
          break;
        case MONO_RED:
          col=getMonochrome(val, Qt::red);
          break;
        case MONO_PURPLE:
          col=getMonochrome(val, Qt::magenta);
          break;
        default:
          col=Qt::black;
          break;
      }
      pen.setColor(col);
      p.setPen(pen);
      p.drawPoint(x,_nrBinsY-y-1);
   }
   return *image;
}

QPixmap QHist2d::getPixmap(HISTO_PALETTE palette, HISTO_GRADATION gradation, bool invert)
{
   QPixmap* pm = new QPixmap(_nrBinsX,_nrBinsY);
//   pm->fill(fBgColor);
//   pm->fill(Qt::white);
   if (!_nrEntries) return *pm;

   QPainter p(pm);
   QPen pen;

   double min=getMinMean();
   double max=getMaxMean();
   if (max==min) {
      if (max<0.) max=0.;
      else min=0.;
   }

   double range=max-min;

   for (int y=0; y<_nrBinsY; y++)
      for (int x=0; x<_nrBinsX; x++)
   {
      if (!GetBinMultiplicity(x,y)) {
         pen.setColor(fBgColor);
         p.setPen(pen);
         p.drawPoint(x,_nrBinsY-y-1);
         continue;
      }
      double val=GetMeanBinContent(x,y);
//      double val=GetBinMultiplicity(x,y);
      val=(val-min)/range;

      if (gradation==LOG) val=log10(val*range+1.)/log10(range+1.);
      if (invert) val=1.-val;
      QColor col;
      switch (palette) {
        case GREYSCALE:
          col=getGreyScale(val);
          break;
        case COLOR1:
          col=getFalseColor(val);
          break;
        case MONO_YELLOW:
          col=getMonochrome(val, Qt::yellow);
          break;
        case MONO_GREEN:
          col=getMonochrome(val, Qt::green);
          break;
        case MONO_BLUE:
          col=getMonochrome(val, Qt::blue);
          break;
        case MONO_RED:
          col=getMonochrome(val, Qt::red);
          break;
        case MONO_PURPLE:
          col=getMonochrome(val, Qt::magenta);
          break;
        default:
          col=Qt::black;
          break;
      }
      pen.setColor(col);
      p.setPen(pen);
      p.drawPoint(x,_nrBinsY-y-1);
   }
   return *pm;
}

QPixmap QHist2d::getPalette(HISTO_PALETTE palette, bool invert)
{
   QPixmap* pm = new QPixmap(1,1024);
//   pm->fill(fBgColor);
//   pm->fill(Qt::white);

   QPainter p(pm);
   QPen pen;

   for (int i=0; i<1024; i++)
   {
      double val=(double)i/1023.;

      QColor col;
      switch (palette) {
        case GREYSCALE:
          col=getGreyScale(val);
          break;
        case COLOR1:
          col=getFalseColor(val);
          break;
        case MONO_YELLOW:
          col=getMonochrome(val, Qt::yellow);
          break;
        case MONO_GREEN:
          col=getMonochrome(val, Qt::green);
          break;
        case MONO_BLUE:
          col=getMonochrome(val, Qt::blue);
          break;
        case MONO_RED:
          col=getMonochrome(val, Qt::red);
          break;
        case MONO_PURPLE:
          col=getMonochrome(val, Qt::magenta);
          break;
        default:
          col=Qt::black;
          break;
      }
      pen.setColor(col);
      p.setPen(pen);
      if (invert) p.drawPoint(0,i);
      else p.drawPoint(0,1023-i);
   }
   return *pm;
}

int QHist2d::toBinX(double value)
{
   return (int)((value-_startX)/(_endX-_startX)*(double)_nrBinsX);
}

int QHist2d::toBinY(double value)
{
   return (int)((value-_startY)/(_endY-_startY)*(double)_nrBinsY);
}

void QHist2d::clear()
{
   if (_data) { delete[] _data; _data=0; }
   _data = new std::vector<double>[_nrBins];
   _nrEntries=_underflowX=_overflowX=_underflowY=_overflowY=0.;
//   _nrBins=_nrBinsX=_nrBinsY=0;
}

double QHist2d::getMin()
{
   double m=1e+38;
   for (int i=0; i<_nrBins; i++)
   {
      double sum = 0.;
      for (std::vector<double>::iterator it = _data[i].begin();
           it!=_data[i].end();
           ++it)
      {
         sum+=*it;
      }
      if (sum<m) m=sum;
   }
   return m;
}

double QHist2d::getMax()
{
   double m=-1e+38;
   for (int i=0; i<_nrBins; i++)
   {
      double sum = 0.;
      for (std::vector<double>::iterator it = _data[i].begin();
           it!=_data[i].end();
           ++it)
      {
         sum+=*it;
      }
      if (sum>m) m=sum;
   }
   return m;
}

double QHist2d::GetMeanContent(double x, double y)
{
   int xbin=toBinX(x);
   int ybin=toBinY(y);
   if (xbin>=0 && ybin>=0 && xbin<_nrBinsX && ybin<_nrBinsY)
      return GetBinContent(xbin,ybin)/(double)_data[ybin*_nrBinsX+xbin].size();
   else return NAN;
}

double QHist2d::GetMeanBinContent(int xbin, int ybin)
{
   if (xbin>=0 && ybin>=0 && xbin<_nrBinsX && ybin<_nrBinsY)
      return GetBinContent(xbin,ybin)/(double)_data[ybin*_nrBinsX+xbin].size();
   else return NAN;
}

double QHist2d::GetMeanBinContent(int bin)
{
   if (bin>=0 && bin<_nrBins)
      return GetBinContent(bin)/(double)_data[bin].size();
   else return NAN;
}

double QHist2d::GetBinContent(int xbin, int ybin)
{
   double sum=0.;
   if (xbin>=0 && ybin>=0 && xbin<_nrBinsX && ybin<_nrBinsY)
      for (std::vector<double>::iterator it = _data[ybin*_nrBinsX+xbin].begin();
         it!=_data[ybin*_nrBinsX+xbin].end();
         ++it)
      {
         sum+=*it;
      }
   return sum;
}

double QHist2d::GetBinContent(int bin)
{
   double sum=0.;
   if (bin>=0 && bin<_nrBins)
      for (std::vector<double>::iterator it = _data[bin].begin();
         it!=_data[bin].end();
         ++it)
      {
         sum+=*it;
      }
   return sum;
}

int QHist2d::GetBinMultiplicity(int xbin, int ybin)
{
   if (xbin>=0 && ybin>=0 && xbin<_nrBinsX && ybin<_nrBinsY)
      return _data[ybin*_nrBinsX+xbin].size();
   return 0;
}


double QHist2d::getMinMean()
{
   double m=INFINITY;
   for (int i=0; i<_nrBins; i++)
   {
      if (!_data[i].size()) continue;
      //m=1e+38;
      double sum = 0.;
      for (std::vector<double>::iterator it = _data[i].begin();
           it!=_data[i].end();
           ++it)
      {
         sum+=*it;
      }
      sum/=(double)_data[i].size();
      if (sum<m) m=sum;
   }
   return m;
}

double QHist2d::getMaxMean()
{
   double m=-INFINITY;
   for (int i=0; i<_nrBins; i++)
   {
      if (!_data[i].size()) continue;
      //m=-1e+38;
      double sum = 0.;
      for (std::vector<double>::iterator it = _data[i].begin();
           it!=_data[i].end();
           ++it)
      {
         sum+=*it;
      }
      sum/=(double)_data[i].size();
      if (sum>m) m=sum;
   }
   return m;
}

QHist2d QHist2d::interpolateBins(int nIterations)
{
   if (nIterations<1) return *this;
   QHist2d result(*this);
   for (int y=0; y<_nrBinsY; y++)
   {
      for (int x=0; x<_nrBinsX; x++) {
          if (GetBinMultiplicity(x,y)) continue;
          double sum=0.;
          int mult=0;
          if (GetBinMultiplicity(x-1,y-1)) { sum+=GetMeanBinContent(x-1,y-1); mult++; }
          if (GetBinMultiplicity(x-1,y)) { sum+=GetMeanBinContent(x-1,y); mult++; }
          if (GetBinMultiplicity(x-1,y+1)) { sum+=GetMeanBinContent(x-1,y+1); mult++; }
          if (GetBinMultiplicity(x,y-1)) { sum+=GetMeanBinContent(x,y-1); mult++; }
          if (GetBinMultiplicity(x,y+1)) { sum+=GetMeanBinContent(x,y+1); mult++; }
          if (GetBinMultiplicity(x+1,y-1)) { sum+=GetMeanBinContent(x+1,y-1); mult++; }
          if (GetBinMultiplicity(x+1,y)) { sum+=GetMeanBinContent(x+1,y); mult++; }
          if (GetBinMultiplicity(x+1,y+1)) { sum+=GetMeanBinContent(x+1,y+1); mult++; }
          if (mult>1) result.fillBin(x,y,sum/(double)mult);
      }
   }
   if (nIterations>1) return result.interpolateBins(nIterations-1);
   return result;
}

QColor QHist2d::getGreyScale(double val)
{
   const int maxintensity=255;
   int b,g,r;
   int v=(int)((double)maxintensity*val);
   if (v<0) v=0;
   if (v>maxintensity) v=maxintensity;
   return QColor(v,v,v);
}

QColor QHist2d::getMonochrome(double val, const QColor& col)
{
   int hue=col.hue();
   const int maxintensity=510;
   int x=(int)((double)maxintensity*val);
   if (x<0) x=0;
   if (x>maxintensity) x=maxintensity;
   if (x<256) return QColor::fromHsv(hue,255,x);
   return QColor::fromHsv(hue,510-x,255);
}

QColor QHist2d::getFalseColor(double val)
{
   const int maxintensity=1791;
   int b,g,r;
   int v=(int)((double)maxintensity*val);
   if (v<0) v=0;
   if (v>maxintensity) v=maxintensity;

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
   else { r=g=b=0; }

   return QColor(r,g,b);
}
