#ifndef CRITERION_H
#define CRITERION_H

#include "abstractevent.h"

#include <memory>

namespace MuonPi {


template <typename T>
/**
 * @brief The Criterion class
 * Abstract class for a relationship between two events
 */
class Criterion
{
public:
    virtual ~Criterion() = default;
    /**
     * @brief criterion Assigns a value of type T to a pair of events
     * @param first The first event to check
     * @param second the second event to check
     * @return a value of type T corresponding to the relationship between both events
     */
    [[nodiscard]] virtual auto criterion(const std::unique_ptr<AbstractEvent>& first, const std::unique_ptr<AbstractEvent>& second) const -> T = 0;
    /**
     * @brief met Checks whether the value corresponds to a valid value or not
     * @param value The value to test
     * @return true if the value corresponds to a valid value, false if not
     */
    [[nodiscard]] virtual auto met(const T& value) const -> bool = 0;
};

}

#endif // CRITERION_H
