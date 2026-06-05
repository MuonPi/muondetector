#ifndef HISTOGRAM_SERIES_DATA_H
#define HISTOGRAM_SERIES_DATA_H

#include <qvector.h>
#include <qwt_samples.h>
#include <qwt_series_data.h>

class HistogramSeriesData : public QwtArraySeriesData<QwtIntervalSample> {
  public:
    HistogramSeriesData() = default;

    explicit HistogramSeriesData(const QVector<QwtIntervalSample>& samples);
    void setSamples(const QVector<QwtIntervalSample>& samples);
    size_t size() const override;

    QwtIntervalSample sample(size_t i) const override;

    QRectF boundingRect() const override;

  private:
    QVector<QwtIntervalSample> m_samples;

    mutable QRectF m_boundingRect;
};

#endif // HISTOGRAM_SERIES_DATA_H