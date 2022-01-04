//#include <QDataStream>
#include <cmath>
#include <map>
#include <string>

#include "histogram.h"

Histogram::Histogram(const std::string& name, int nrBins, double min, double max)
    : fName(name)
    , fNrBins(nrBins)
    , fMin(min)
    , fMax(max)
{
}

Histogram::~Histogram()
{
    fHistogramMap.clear();
}

void Histogram::clear()
{
    fHistogramMap.clear();
    fUnderflow = fOverflow = 0.;
}

void Histogram::setName(const std::string& name)
{
    fName = name;
}

void Histogram::setUnit(const std::string& unit)
{
    fUnit = unit;
}

void Histogram::setNrBins(int bins)
{
    fNrBins = bins;
    clear();
}

int Histogram::getNrBins() const
{
    return fNrBins;
}

void Histogram::setMin(double val)
{
    fMin = val;
}

double Histogram::getMin() const
{
    return fMin;
}

void Histogram::setMax(double val)
{
    fMax = val;
}

double Histogram::getMax() const
{
    return fMax;
}

double Histogram::getRange() const
{
    return fMax - fMin;
}

double Histogram::getCenter() const
{
    return 0.5 * getRange() + fMin;
}

double Histogram::getBinCenter(int bin) const
{
    return bin2Value(bin);
}

int Histogram::getLowestOccupiedBin() const
{
    if (fHistogramMap.empty())
        return -1;
    auto it = fHistogramMap.begin();
    while (it != fHistogramMap.end() && it->second < 1e-3)
        ++it;
    if (it == fHistogramMap.end())
        return -1;
    return it->first;
}

int Histogram::getHighestOccupiedBin() const
{
    if (fHistogramMap.empty())
        return -1;
    auto it = --fHistogramMap.end();
    while (it != fHistogramMap.begin() && it->second < 1e-3)
        --it;
    if (it == fHistogramMap.begin())
        return 0;
    return it->first;
}

void Histogram::fill(double x, double mult)
{
	int bin = value2Bin(x);
	if ( fAutoscale && getEntries() < 1e-6 ) {
		// initial fill, so lets see if we should rescale to the first value
		if ( bin < 0 || bin >= fNrBins ) {
			rescale( x );
			fill( x, mult );
			return;
		}
	}
	if (bin < 0) {
        fUnderflow += mult;
    } else if (bin >= fNrBins) {
        fOverflow += mult;
    } else
        fHistogramMap[bin] += mult;
}

void Histogram::setBinContent(int bin, double value)
{
    if (bin >= 0 && bin < fNrBins)
        fHistogramMap[bin] = value;
}

double Histogram::getBinContent(int bin) const
{
    if (bin >= 0 && bin < fNrBins) {
        try {
            auto it = fHistogramMap.find(bin);
            if (it != fHistogramMap.end())
                return fHistogramMap.at(bin);
            else
                return double();
        } catch (...) {
            return double();
        }
    } else
        return double();
}

double Histogram::getMean()
{
    double sum = 0., entries = 0.;
    for (const auto& entry : fHistogramMap) {
        entries += entry.second;
        sum += bin2Value(entry.first) * entry.second;
    }
    if (entries > 0.)
        return sum / entries;
    else
        return 0.;
}

double Histogram::getRMS()
{
    double mean = getMean();
    double sum = 0., entries = 0.;
    for (const auto& entry : fHistogramMap) {
        entries += entry.second;
        double dx = bin2Value(entry.first) - mean;
        sum += dx * dx * entry.second;
    }
    if (entries > 1.)
        return sqrt(sum / (entries - 1.));
    else
        return 0.;
}

double Histogram::getUnderflow() const
{
    return fUnderflow;
}

double Histogram::getOverflow() const
{
    return fOverflow;
}

double Histogram::getEntries()
{
    double sum = fUnderflow + fOverflow;
    for (const auto& entry : fHistogramMap) {
        sum += entry.second;
    }
    return sum;
}

int Histogram::value2Bin(double value) const
{
    double range = fMax - fMin;
    if (range <= 0.)
        return -1;
    int bin = (value - fMin) / range * (fNrBins - 1) + 0.5;
    return bin;
}

double Histogram::bin2Value(int bin) const
{
    double range = fMax - fMin;
    if (range <= 0.)
        return -1;
    double value = range * bin / (fNrBins - 1) + fMin;
    return value;
}

void Histogram::rescale(double center, double width)
{
    setMin(center - width / 2.);
    setMax(center + width / 2.);
    clear();
}

void Histogram::rescale(double center)
{
    double width = getMax() - getMin();
    rescale(center, width);
}

void Histogram::rescale()
{
    if ( !fAutoscale ) return;
	
    // Strategy: check if more than 1% of all entries in underflow/overflow
    // set new center to old center, adjust range by 20%
    // set center to newValue if histo empty or only underflow/overflow filled
    // histo will not be filled with supplied value, it has to be done externally
    double entries = getEntries();
    // do nothing if histo is empty
    if ( entries < 3. ) {
        return;
    }
    double ufl = getUnderflow();
    double ofl = getOverflow();
    entries -= ufl + ofl;
    double range = getMax() - getMin();
	int lowest = getLowestOccupiedBin();
	int highest = getHighestOccupiedBin();
	double lowestEntry = getBinCenter(lowest);
	double highestEntry = getBinCenter(highest);
    if (ufl > 0. && ofl > 0. && (ufl + ofl) > 0.01 * entries) {
        // range is too small, underflow and overflow have more than 1% of all entries
        rescale( 0.5 * ( highestEntry-lowestEntry ) + lowestEntry, 1.2 * range );
    } else if ( ufl > 0.005 * entries ) {
		setMin( getMax() - range * 1.2 );
		clear();
    } else if ( ofl > 0.005 * entries ) {
		setMax( getMin() + range * 1.2 );
		clear();
    } else if ( ufl < 1e-3 && ofl < 1e-3 ) {
        // check if range is too wide
        if ( entries > 1000. && static_cast<double>(highest-lowest)/fNrBins < 0.05 ) {
            rescale( 0.5 * ( highestEntry-lowestEntry ) + lowestEntry, 0.8 * range );
        }
    }
}

void Histogram::setAutoscale( bool autoscale )
{
	fAutoscale = autoscale;
}
