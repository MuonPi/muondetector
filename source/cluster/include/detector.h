#ifndef DETECTOR_H
#define DETECTOR_H


#include <memory>

namespace MuonPi {


class RateSupervisor;

class RateListener
{
public:
    virtual void factor_changed(std::size_t hash, float factor) = 0;
};

/**
 * @brief The Detector class
 * Represents a connected detector.
 * Scans the message rate and deletes the runtime objects from memory if the detector has not been active for some time.
 */
class Detector
{
public:
private:
    std::size_t m_hash { 0 };
    std::unique_ptr<RateSupervisor> m_supervisor { nullptr };
    RateListener* m_listener { nullptr };
};

}

#endif // DETECTOR_H
