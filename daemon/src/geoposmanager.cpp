#include "geoposmanager.h"
#include <QDebug>

static constexpr double pi() { return 3.14159265358979; }
static constexpr double sqrt2 { 1.414213562373095 };
static double sqr(double x) { return x * x; }
constexpr size_t MIN_HISTO_ENTRIES { 10 };

GeoPosManager::GeoPosManager(const PositionModeConfig& mode_config)
    : m_mode_config(mode_config)
{
}

void GeoPosManager::set_mode_config(const PositionModeConfig& mode_config)
{
    if (mode_config.filter_config != m_mode_config.filter_config) {
        switch (mode_config.filter_config) {
        case PositionModeConfig::FilterType::Kalman:
            m_gnss_pos_kalman.reset();
            break;
        default:
            break;
        }
    }
    m_mode_config = mode_config;
    if (m_mode_config.mode == PositionModeConfig::Mode::Static) {
        m_position = m_mode_config.static_position;
    }
}

auto GeoPosManager::get_mode_config() const -> const PositionModeConfig&
{
    return m_mode_config;
}

void GeoPosManager::set_histos(
    std::shared_ptr<Histogram> lon,
    std::shared_ptr<Histogram> lat,
    std::shared_ptr<Histogram> height)
{
    m_lon_histo = lon;
    m_lat_histo = lat;
    m_height_histo = height;
}

void GeoPosManager::new_position(const GeoPosition& new_pos)
{
    bool valid_lock_in_candidate { true };
    const double totalPosAccuracy = new_pos.pos_error();
    GeoPosition preliminary_pos {};
    switch (m_mode_config.filter_config) {
    case PositionModeConfig::FilterType::None:
        preliminary_pos = new_pos;
        break;
    case PositionModeConfig::FilterType::Kalman:
        m_gnss_pos_kalman.process(new_pos.latitude, new_pos.longitude, new_pos.altitude, totalPosAccuracy);
        preliminary_pos = {
            m_gnss_pos_kalman.get_longitude(),
            m_gnss_pos_kalman.get_latitude(),
            m_gnss_pos_kalman.get_altitude(),
            m_gnss_pos_kalman.get_accuracy() / sqrt2,
            m_gnss_pos_kalman.get_accuracy() / sqrt2
        };
        break;
    case PositionModeConfig::FilterType::HistoMpv:
    case PositionModeConfig::FilterType::HistoMedian:
    case PositionModeConfig::FilterType::HistoMean:
        preliminary_pos = get_pos_from_histos();
        if (m_mode_config.mode == PositionModeConfig::Mode::LockIn) {
            if (m_height_histo->getEntries() < MuonPi::Config::lock_in_min_histogram_entries
                || m_lat_histo->getEntries() < MuonPi::Config::lock_in_min_histogram_entries
                || m_lon_histo->getEntries() < MuonPi::Config::lock_in_min_histogram_entries) {
                valid_lock_in_candidate = false;
            }
            if (m_height_histo->getEntries() > MuonPi::Config::lock_in_max_histogram_entries) {
                m_height_histo->clear();
            }
            if (m_lat_histo->getEntries() > MuonPi::Config::lock_in_max_histogram_entries) {
                m_lat_histo->clear();
            }
            if (m_lon_histo->getEntries() > MuonPi::Config::lock_in_max_histogram_entries) {
                m_lon_histo->clear();
            }
        }
    default:
        break;
    }

    if (!preliminary_pos.valid()) {
        return;
    }
    // send new position only if it was found valid, depending on filter
    if (m_mode_config.mode == PositionModeConfig::Mode::Auto) {
        m_position = preliminary_pos;
        if (m_valid_pos_fn) {
            m_valid_pos_fn(m_position);
        }
        return;
    }

    if (valid_lock_in_candidate) {
        check_for_lockin_reached(preliminary_pos);
    }
}

GeoPosition GeoPosManager::get_pos_from_histos() const
{
    constexpr double earth_radius_meters { 6367444.5 };
    const double degree_to_surface_meters {
        (pi() * 2 * earth_radius_meters) / 360.
    };

    if ((m_height_histo && m_height_histo->getEntries() < MIN_HISTO_ENTRIES)
        || (m_lon_histo && m_lon_histo->getEntries() < MIN_HISTO_ENTRIES)
        || (m_lat_histo && m_lat_histo->getEntries() < MIN_HISTO_ENTRIES)) {
        return GeoPosition {};
    }

    GeoPosition preliminary_pos {};
    switch (m_mode_config.filter_config) {
    case PositionModeConfig::FilterType::HistoMean:
        preliminary_pos.altitude = m_height_histo->getMean();
        preliminary_pos.latitude = m_lat_histo->getMean();
        preliminary_pos.longitude = m_lon_histo->getMean();
        break;
    case PositionModeConfig::FilterType::HistoMedian:
        preliminary_pos.altitude = m_height_histo->getMedian();
        preliminary_pos.latitude = m_lat_histo->getMedian();
        preliminary_pos.longitude = m_lon_histo->getMedian();
        break;
    case PositionModeConfig::FilterType::HistoMpv:
        preliminary_pos.altitude = m_height_histo->getMpv();
        preliminary_pos.latitude = m_lat_histo->getMpv();
        preliminary_pos.longitude = m_lon_histo->getMpv();
        break;
    default:
        return GeoPosition {};
    }
    preliminary_pos.vert_error = m_height_histo->getRMS();
    preliminary_pos.hor_error = m_lat_histo->getRMS() * degree_to_surface_meters;
    preliminary_pos.hor_error *= preliminary_pos.hor_error;
    preliminary_pos.hor_error += sqr(m_lon_histo->getRMS() * degree_to_surface_meters / pi() * preliminary_pos.latitude / 180.);
    preliminary_pos.hor_error = std::sqrt(preliminary_pos.hor_error);
    return preliminary_pos;
}

void GeoPosManager::check_for_lockin_reached(const GeoPosition& preliminary_pos)
{
    if (m_mode_config.mode != PositionModeConfig::Mode::LockIn)
        return;
    if (preliminary_pos.vert_error < m_mode_config.lock_in_min_error_meters
        && preliminary_pos.hor_error < m_mode_config.lock_in_min_error_meters) {
        m_mode_config.mode = PositionModeConfig::Mode::Static;
        m_mode_config.static_position = { preliminary_pos };
        if (m_lockin_ready_fn) {
            m_lockin_ready_fn(preliminary_pos);
        }
        qInfo() << "concluded geo pos lock-in and fixed position: lat=" << preliminary_pos.latitude
                << "lon=" << preliminary_pos.longitude
                << "(err=" << preliminary_pos.hor_error << "m)"
                << "alt=" << preliminary_pos.altitude
                << "(err=" << preliminary_pos.vert_error << "m)";
    }
}

const GeoPosition& GeoPosManager::get_current_position() const
{
    return m_position;
}

void GeoPosManager::set_static_position(const GeoPosition& pos)
{
    m_mode_config.static_position = pos;
}

const GeoPosition& GeoPosManager::get_static_position() const
{
    return m_mode_config.static_position;
}

void GeoPosManager::set_mode(PositionModeConfig::Mode mode)
{
    m_mode_config.mode = mode;
}

auto GeoPosManager::get_mode() const -> PositionModeConfig::Mode
{
    return m_mode_config.mode;
}

void GeoPosManager::set_filter(PositionModeConfig::FilterType filter)
{
    m_mode_config.filter_config = filter;
}

auto GeoPosManager::get_filter() const -> PositionModeConfig::FilterType
{
    return m_mode_config.filter_config;
}

void GeoPosManager::set_lockin_ready_callback(std::function<void(GeoPosition)> func)
{
    m_lockin_ready_fn = func;
}

void GeoPosManager::set_valid_pos_callback(std::function<void(GeoPosition)> func)
{
    m_valid_pos_fn = func;
}
