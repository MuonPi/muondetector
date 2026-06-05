#include "curve_series_data.h"

#include <limits>
#include <qpoint.h>
#include <qvector.h>

CurveSeriesData::CurveSeriesData(const QVector<QPointF>& samples) {
    setSamples(samples);
}

void CurveSeriesData::setSamples(const QVector<QPointF>& samples) {
    m_samples = samples;

    // invalidate cached bounding rect (Qwt base class)
    m_boundingRect = QRectF();
}

const QVector<QPointF>& CurveSeriesData::samples() const {
    return m_samples;
}

size_t CurveSeriesData::size() const {
    return static_cast<size_t>(m_samples.size());
}

QPointF CurveSeriesData::sample(size_t i) const {
    return m_samples[static_cast<int>(i)];
}

QRectF CurveSeriesData::boundingRect() const {
    if (m_boundingRect.isValid())
        return m_boundingRect;

    if (m_samples.isEmpty())
        return QRectF();

    double xmin = std::numeric_limits<double>::max();
    double xmax = std::numeric_limits<double>::lowest();
    double ymin = std::numeric_limits<double>::max();
    double ymax = std::numeric_limits<double>::lowest();

    for (const auto& p : m_samples) {
        xmin = std::min(xmin, p.x());
        xmax = std::max(xmax, p.x());
        ymin = std::min(ymin, p.y());
        ymax = std::max(ymax, p.y());
    }

    m_boundingRect = QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));

    return m_boundingRect;
}
