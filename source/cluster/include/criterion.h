#ifndef CRITERION_H
#define CRITERION_H


#include <memory>

namespace MuonPi {

class Event;

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
    [[nodiscard]] virtual auto criterion(const std::unique_ptr<Event>& first, const std::unique_ptr<Event>& second) const -> bool = 0;
};

}

#endif // CRITERION_H
