#ifndef GEOPOSMANAGER_H
#define GEOPOSMANAGER_H
#include "utility/kalman_gnss_filter.h"

#include <cmath>
#include <config.h>
#include <functional>
#include <histogram.h>
#include <memory>
#include <muondetector_structs.h>

class GeoPosManager {
  public:
    GeoPosManager() = default;
    GeoPosManager(const PositionModeConfig& mode_config);
    ~GeoPosManager() = default;

    // Used to fill new positional data from GPS
    void fill_from_gps(const GnssPosStruct& event);

    // Fill in status data from GPS
    void set_fix_status(std::uint8_t gpsFix);
    void set_time_accuracy(std::uint32_t tAcc);

    // Fill in / retrieve configuration from UI / settings file
    void set_mode_config(const PositionModeConfig& mode_config);
    auto get_mode_config() const -> const PositionModeConfig&;
    auto get_mode() const -> PositionModeConfig::Mode;

    // Set callback for lockin ready and valid pos
    void set_lockin_ready_callback(std::function<void(GeoPosition)> func);
    void set_valid_pos_callback(std::function<void(GeoPosition)> func);

    // Direct access to currently stored position data
    void set_static_position(const GeoPosition& pos);
    const GeoPosition& get_current_position() const;
    const GeoPosition& get_static_position() const;

    // Used by datastore
    void set_histos(std::shared_ptr<Histogram> lon, std::shared_ptr<Histogram> lat,
                    std::shared_ptr<Histogram> height);

  private:
    void new_position(const GeoPosition& new_pos);
    void set_mode(PositionModeConfig::Mode mode);
    void set_filter(PositionModeConfig::FilterType filter);
    auto get_filter() const -> PositionModeConfig::FilterType;

    Property<Gnss::FixType>& fix_status();
    Property<std::chrono::nanoseconds> time_precision();

    GeoPosition get_pos_from_histos() const;
    void check_for_lockin_reached(const GeoPosition& preliminary_pos);

    GeoPosition m_position{};
    PositionModeConfig m_mode_config{};
    std::function<void(GeoPosition)> m_lockin_ready_fn;
    std::function<void(GeoPosition)> m_valid_pos_fn;

    KalmanGnssFilter m_gnss_pos_kalman{0.1};
    std::shared_ptr<Histogram> m_lon_histo;
    std::shared_ptr<Histogram> m_lat_histo;
    std::shared_ptr<Histogram> m_height_histo;
    Property<Gnss::FixType> m_fix_status{};
    Property<std::chrono::nanoseconds> m_time_precision{};
};

#endif // GEOPOSMANAGER_H
