#ifndef COINCIDENCE_H
#define COINCIDENCE_H

#include "criterion.h"

#include <memory>
#include <chrono>

namespace MuonPi {


/**
 * @brief The Coincidence class
 * Defines the parameters for a coincidence between two events
 */
class Coincidence : public Criterion<bool>
{
public:
    ~Coincidence() override;
    /**
     * @brief criterion Assigns a value of type T to a pair of events
     * @param first The first event to check
     * @param second the second event to check
     * @return true if the events have a coincidence
     */
    [[nodiscard]] auto criterion(const std::unique_ptr<AbstractEvent>& first, const std::unique_ptr<AbstractEvent>& second) const -> bool override;
    /**
     * @brief met Checks whether the value corresponds to a valid value or not
     * @param value The value to test
     * @return true if the value corresponds to a valid value, false if not
     */
    [[nodiscard]] auto met(const bool& value) const -> bool override;

private:
    [[maybe_unused]] std::chrono::steady_clock::duration m_time {};
};

}

#endif // COINCIDENCE_H
