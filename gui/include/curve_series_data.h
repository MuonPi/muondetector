#ifndef CURVE_SERIES_DATA_H
#define CURVE_SERIES_DATA_H

#include <qpoint.h>
#include <qrect.h>
#include <qvector.h>
#include <qwt_series_data.h>

class CurveSeriesData : public QwtSeriesData<QPointF> {
  public:
    CurveSeriesData() = default;
    explicit CurveSeriesData(const QVector<QPointF>& samples);

    void setSamples(const QVector<QPointF>& samples);
    const QVector<QPointF>& samples() const;

    size_t size() const override;
    QPointF sample(size_t i) const override;
    QRectF boundingRect() const override;

  private:
    QVector<QPointF> m_samples;

    mutable QRectF m_boundingRect;
};

#endif // CURVE_SERIES_DATA_H