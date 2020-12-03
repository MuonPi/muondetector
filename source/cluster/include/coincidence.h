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
    [[nodiscard]] auto criterion(const std::unique_ptr<Event>& first, const std::unique_ptr<Event>& second) const -> float override;

    /**
     * @brief maximum_false
     * @return The upper limit where the criterion is false.
     */
    [[nodiscard]] auto maximum_false() const -> float override
    {
        return -3.5f;
    }

    /**
     * @brief minimum_true
     * @return The lower limit where the criterion is true.
     */
    [[nodiscard]] auto minimum_true() const -> float override
    {
        return 3.5f;
    }

private:
    [[nodiscard]] auto compare(std::chrono::steady_clock::time_point first, std::chrono::steady_clock::time_point second) const -> float;

    std::chrono::steady_clock::duration m_time { std::chrono::microseconds{100} };
};

}

#endif // COINCIDENCE_H
