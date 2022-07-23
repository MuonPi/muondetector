#ifndef KALMAN_GNSS_H
#define KALMAN_GNSS_H

#include <QObject>
#include <QString>
#include <chrono>
#include <cmath>

class KalmanGnssFilter {
public:
    KalmanGnssFilter() = delete;
    KalmanGnssFilter(double accuracy_decay = 1.);

    /// <summary>
    /// Kalman filter processing for lattitude and longitude
    /// </summary>
    /// <param name="lat_measurement_degrees">new measurement of lattitude</param>
    /// <param name="lng_measurement">new measurement of longitude</param>
    /// <param name="alt_measurement">new measurement of altitude</param>
    /// <param name="accuracy">measurement of 1 standard deviation error in metres</param>
    /// <returns>new state</returns>
    void process(double lat_measurement, double lng_measurement, double alt_measurement, double accuracy);
    [[nodiscard]] auto get_timestamp() const -> std::chrono::time_point<std::chrono::steady_clock>;
    [[nodiscard]] auto get_latitude() const -> double;
    [[nodiscard]] auto get_longitude() const -> double;
    [[nodiscard]] auto get_altitude() const -> double;
    [[nodiscard]] auto get_accuracy() const -> double;
    [[nodiscard]] auto valid() const -> bool;
    void reset();

private:
    static constexpr double c_min_accuracy { 1. };
    double m_accuracy_decay { 1. }; ///< free parameter describing the decay of accuracy over time, unit: meters per second

    std::chrono::time_point<std::chrono::steady_clock> m_timestamp {};
    double m_lat { 0. };
    double m_lng { 0. };
    double m_alt { 0. };

    double m_variance; // P matrix.  Negative means object uninitialised.  NB: units irrelevant, as long as same units used throughout

    void set_state(double lat, double lng, double alt, double accuracy);
};

#endif // KALMAN_GNSS_H
