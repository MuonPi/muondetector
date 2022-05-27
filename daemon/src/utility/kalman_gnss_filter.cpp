#include <iostream>
#include "utility/kalman_gnss_filter.h"

/**
 * Kalman GNSS Filter
 */
KalmanGnssFilter::KalmanGnssFilter(double accuracy_decay)
    : m_accuracy_decay( accuracy_decay ), m_variance { -1. }
{ 
}

void KalmanGnssFilter::set_state(double lat, double lng, double alt, double accuracy) {
    m_lat = lat;
    m_lng = lng;
    m_alt = alt;
    m_variance = accuracy * accuracy;
    m_timestamp = std::chrono::steady_clock::now();
}

void KalmanGnssFilter::process(double lat_measurement, double lng_measurement, double alt_measurement, double accuracy) {
	double local_accuracy { accuracy };
	if ( accuracy < c_min_accuracy) local_accuracy = c_min_accuracy;
	if (m_variance < 0) {
		// if variance < 0, object is unitialised, so initialise with current values
		set_state( lat_measurement, lng_measurement, alt_measurement, local_accuracy );
	} else {
		// else apply Kalman filter methodology
		auto current_time = std::chrono::steady_clock::now();
		unsigned int time_increment_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - m_timestamp).count();
		if ( time_increment_ms > 0 ) {
			// time has moved on, so the uncertainty in the current position increases
			m_variance += time_increment_ms * m_accuracy_decay * m_accuracy_decay / 1000;
			m_timestamp = current_time;
			// TO DO: USE VELOCITY INFORMATION HERE TO GET A BETTER ESTIMATE OF CURRENT POSITION
		}

		// Kalman gain matrix K = Covariance * Inverse(Covariance + MeasurementVariance)
		// NB: because K is dimensionless, it doesn't matter that variance has different units to lat and lng
		double K = m_variance / ( m_variance + local_accuracy * local_accuracy);
		// apply K
		m_lat += K * (lat_measurement - m_lat);
		m_lng += K * (lng_measurement - m_lng);
		m_alt += K * (alt_measurement - m_alt);
		// new Covariance  matrix is (IdentityMatrix - K) * Covariance
		m_variance = (1. - K) * m_variance;
	}
}
