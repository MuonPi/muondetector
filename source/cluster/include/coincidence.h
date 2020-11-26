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
class Coincidence : public Criterion
{
public:
    ~Coincidence() override;
    /**
     * @brief criterion Assigns a value of type T to a pair of events
     * @param first The first event to check
     * @param second the second event to check
     * @return true if the events have a coincidence
     */
    [[nodiscard]] auto criterion(const std::unique_ptr<Event>& first, const std::unique_ptr<Event>& second) const -> bool override;

private:
    [[nodiscard]] auto compare(std::chrono::steady_clock::time_point first, std::chrono::steady_clock::time_point second) const -> bool;

    std::chrono::steady_clock::duration m_time { std::chrono::microseconds{100} };
};

}

#endif // COINCIDENCE_H
