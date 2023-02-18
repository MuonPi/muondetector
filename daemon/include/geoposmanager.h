#ifndef GEOPOSMANAGER_H
#define GEOPOSMANAGER_H
#include <functional>
#include <memory>
#include <cmath>
#include <config.h>
#include <muondetector_structs.h>
#include <histogram.h>
#include "utility/kalman_gnss_filter.h"

class GeoPosManager {
public:
    GeoPosManager() = default;
    GeoPosManager( const PositionModeConfig& mode_config );
    ~GeoPosManager() = default;
    void set_mode_config( const PositionModeConfig& mode_config );
    auto get_mode_config() const -> const PositionModeConfig&;
    void set_histos(
        std::shared_ptr<Histogram> lon,
        std::shared_ptr<Histogram> lat,
        std::shared_ptr<Histogram> height 
    );
    void new_position(const GeoPosition& new_pos);
    const GeoPosition& get_current_position() const;
    void set_static_position( const GeoPosition& pos );
    const GeoPosition& get_static_position() const;
    void set_mode( PositionModeConfig::Mode mode );
    auto get_mode() const -> PositionModeConfig::Mode;
    void set_filter(PositionModeConfig::FilterType filter);
    auto get_filter() const -> PositionModeConfig::FilterType;
    void set_lockin_ready_callback(std::function<void(GeoPosition)> func);
    void set_valid_pos_callback(std::function<void(GeoPosition)> func);
    
private:
    GeoPosition m_position {};
    PositionModeConfig m_mode_config {};
    std::function<void(GeoPosition)> m_lockin_ready_fn;
    std::function<void(GeoPosition)> m_valid_pos_fn;

    KalmanGnssFilter m_gnss_pos_kalman { 0.1 };
    std::shared_ptr<Histogram> m_lon_histo;
    std::shared_ptr<Histogram> m_lat_histo;
    std::shared_ptr<Histogram> m_height_histo;
};

#endif // GEOPOSMANAGER_H
