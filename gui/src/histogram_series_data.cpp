#include "histogram_series_data.h"

#include <qvector.h>
#include <qwt_samples.h>
#include <qwt_series_data.h>

HistogramSeriesData::HistogramSeriesData(const QVector<QwtIntervalSample>& samples)
    : m_samples(samples) {
    m_boundingRect = QRectF(); // invalidate cache
}

void HistogramSeriesData::setSamples(const QVector<QwtIntervalSample>& samples) {
    m_samples = samples;
    m_boundingRect = QRectF();
}

size_t HistogramSeriesData::size() const {
    return static_cast<size_t>(m_samples.size());
}

QwtIntervalSample HistogramSeriesData::sample(size_t i) const {
    return m_samples[static_cast<int>(i)];
}

QRectF HistogramSeriesData::boundingRect() const {
    if (m_boundingRect.isValid())
        return m_boundingRect;

    if (m_samples.isEmpty())
        return QRectF();

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto& s : m_samples) {
        minX = std::min(minX, s.interval.minValue());
        maxX = std::max(maxX, s.interval.maxValue());
        maxY = std::max(maxY, s.value);
    }

    m_boundingRect = QRectF(minX, 0.0, maxX - minX, maxY);
    return m_boundingRect;
}