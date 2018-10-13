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

#ifndef QHISTO_H
#define QHISTO_H

#include <QObject>
#include <QColor>
#include <vector>

class QImage;
class QPixmap;
//class std::vector<double>;

class QHist2d : public QObject
{
Q_OBJECT
   public:
      // enums
      enum HISTO_PALETTE { GREYSCALE, COLOR1, SPECTRUM,
                           MONO_YELLOW, MONO_GREEN, MONO_BLUE, MONO_RED, MONO_PURPLE };
      enum HISTO_GRADATION { LINEAR, LOG, SQUARE, SQUARE_ROOT };

      // Constructors, Destructor
      QHist2d();
      QHist2d(const QHist2d& a_hist);
      QHist2d(int nrBinsX, double startX, double endX,
              int nrBinsY, double startY, double endY);
      ~QHist2d();

      // Accessors
      int nrBins() const { return _nrBinsX*_nrBinsY; }
      int nrBinsX() const { return _nrBinsX; }
      int nrBinsY() const { return _nrBinsY; }
      double nrEntries() const { return _nrEntries; }
      double startX() const { return _startX; }
      double endX() const { return _endX; }
      double startY() const { return _startY; }
      double endY() const { return _endY; }
      double underflowX() const { return _underflowX; }
      double overflowX() const { return _overflowX; }
      double underflowY() const { return _underflowY; }
      double overflowY() const { return _overflowY; }
      double GetMeanContent(double x, double y);
      double GetMeanBinContent(int xbin, int ybin);
      double GetMeanBinContent(int bin);
      double GetBinContent(int xbin, int ybin);
      double GetBinContent(int bin);
      int GetBinMultiplicity(int xbin, int ybin);

      void fill(double valueX, double valueY, const double mult=1.);
      void fillBin(int xbin, int ybin, const double mult=1.);

      void fillGauss(double valueX, double valueY, double mult, double sigma);

      QImage getImage(HISTO_PALETTE palette = COLOR1, HISTO_GRADATION gradation = LINEAR, bool invert = false);
      QPixmap getPixmap(HISTO_PALETTE palette = COLOR1, HISTO_GRADATION gradation = LINEAR, bool invert = false);
      QPixmap getPalette(HISTO_PALETTE palette, bool invert=false);
      double getMin();
      double getMax();
      double getMinMean();
      double getMaxMean();
      int toBinX(double value);
      int toBinY(double value);
      const QColor& bgColor() const { return fBgColor; }

      QHist2d interpolateBins(int nIterations=1);

      // operators
      QHist2d& operator=(const QHist2d& a_hist);

   public slots:
      void clear();
      void setBgColor(const QColor& color) { fBgColor = color; }

   private:
      int _nrBins;
      int _nrBinsX;
      int _nrBinsY;
      double _nrEntries;
      double _startX;
      double _endX;
      double _startY;
      double _endY;
      QColor fBgColor;
//      double* _data;
      // todo
      std::vector<double>* _data;

      double _underflowX,_overflowX,_underflowY,_overflowY;

      QColor getFalseColor(double val);
      QColor getGreyScale(double val);
      QColor getMonochrome(double val, const QColor& col);
};

#endif
