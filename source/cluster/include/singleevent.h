#ifndef SINGLEEVENT_H
#define SINGLEEVENT_H

#include "abstractevent.h"

#include <string>

namespace MuonPi {
/**
 * @brief The SingleEvent class
 * It holds a single event, which originates from an individual Detector.
 */
class SingleEvent : public AbstractEvent
{
public:
    ~SingleEvent() override;
    /**
     * @brief site_id
     * @return The site_id of the detector
     */
    [[nodiscard]] auto site_id() const -> std::string;
    /**
     * @brief username
     * @return The username of the owner of the detector
     */
    [[nodiscard]] auto username() const -> std::string;

private:
    std::string m_site_id {};
    std::string m_username {};
};
}

#endif // SINGLEEVENT_H
