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
    [[nodiscard]] auto criterion(const Event& first, const Event& second) const -> double override;

    /**
     * @brief maximum_false
     * @return The upper limit where the criterion is false.
     */
    [[nodiscard]] auto maximum_false() const -> double override
    {
        return -3.5;
    }

    /**
     * @brief minimum_true
     * @return The lower limit where the criterion is true.
     */
    [[nodiscard]] auto minimum_true() const -> double override
    {
        return 3.5;
    }

private:
    /**
     * @brief compare Compare two timestaps to each other
     * @param difference difference between both timestamps
     * @return returns a value indicating the coincidence time between the two timestamps. @see maximum_fals @see minimum_true for the limits of the values.
     */
    [[nodiscard]] auto compare(std::int_fast64_t start, std::int_fast64_t end) const -> double;

    std::int_fast64_t m_time { 100000 };
};

}

#endif // COINCIDENCE_H
