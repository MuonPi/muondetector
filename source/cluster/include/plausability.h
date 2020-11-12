#ifndef PLAUSABILITY_H
#define PLAUSABILITY_H

#include "criterion.h"

#include <memory>

namespace MuonPi {


/**
 * @brief The Plausability class
 * Defines the parameters to define the plausability for a coincidence between two events
 */
class Plausability : public Criterion<double>
{
public:
    ~Plausability() override;
    /**
     * @brief criterion Assigns a value of type T to a pair of events
     * @param first The first event to check
     * @param second the second event to check
     * @return double value indicating the plausability
     */
    [[nodiscard]] auto criterion(const std::unique_ptr<AbstractEvent>& first, const std::unique_ptr<AbstractEvent>& second) const -> double override;
    /**
     * @brief met Checks whether a plausability value corresponds to a valid value or not
     * @param value The value to test
     * @return true if the value corresponds to a valid value, false if not
     */
    [[nodiscard]] auto met(const double& value) const -> bool override;
};

}

#endif // PLAUSABILITY_H
