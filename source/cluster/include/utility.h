#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <array>

namespace MuonPi {

class MessageConstructor
{
public:
    /**
     * @brief MessageConstructor
     * @param delimiter The delimiter which separates the fields
     */
    MessageConstructor(char delimiter);

    /**
     * @brief add_field Adds a field to the complete message
     * @param field The field to add
     */
    void add_field(const std::string& field);

    /**
     * @brief get_string Gets the complete string
     * @return The completed string
     */
    [[nodiscard]] auto get_string() const -> std::string;

private:
    std::string m_message {};
    char m_delimiter;
};


class MessageParser
{
public:
    /**
     * @brief MessageParser
     * @param message The message to parse
     * @param delimiter The delimiter separating the fields in the message
     */
    MessageParser(const std::string& message, char delimiter);

    /**
     * @brief size
     * @return The number of fields in the message
     */
    [[nodiscard]] auto size() const -> std::size_t;
    /**
     * @brief empty
     * @return true if there are no fields
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @brief operator [] Access one field in the message
     * @param i The index of the field
     * @return The string contained in the field
     */
    [[nodiscard]] auto operator[](std::size_t i) const -> std::string;

private:
    /**
     * @brief skip_delimiter Skips all delimiters until the next field
     */
    void skip_delimiter();
    /**
     * @brief read_field reads the next field
     */
    void read_field();
    /**
     * @brief at_end
     * @return True if the iterator is at the end of the string
     */
    [[nodiscard]] auto at_end() -> bool;

    std::string m_content {};
    std::string::iterator m_iterator;
    char m_delimiter {};

    std::vector<std::pair<std::string::iterator, std::string::iterator>> m_fields {};
};

template <std::size_t N, std::size_t T>
class RateMeasurement
{
public:
    /**
     * @brief increase_counter Increases the counter in the current interval
     */
    void increase_counter();

    /**
     * @brief step Called periodically
     * @return True if the timeout was reached and the rates have been determined in this step
     */
    auto step() -> bool;

    /**
     * @brief current Get the current rate
     * @return The current rate. Might be a little unstable
     */
    [[nodiscard]] auto current() const noexcept -> double;

    /**
     * @brief mean The mean rate over the specified interval
     * @return The mean rate
     */
    [[nodiscard]] auto mean() const noexcept -> double;

    /**
     * @brief deviation Standar deviation
     * @return The standard deviation of all entries in the current interval used for the mean
     */
    [[nodiscard]] auto deviation() const noexcept -> double;

private:
    double m_current { 0.0 };
    double m_mean { 0.0 };
    double m_variance { 0.0 };
    double m_deviation { 0.0 };

    double m_full { false };

    std::array<double, N> m_history { 0.0 };

    std::size_t m_index { 0 };

    std::size_t m_current_n { 0 };
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};


template <std::size_t N, std::size_t T>
void RateMeasurement<N, T>::increase_counter()
{
    m_current_n++;
}

template <std::size_t N, std::size_t T>
auto RateMeasurement<N, T>::step() -> bool
{
    std::chrono::steady_clock::time_point now { std::chrono::steady_clock::now() };
    if (static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last).count()) >= T) {
        m_last = now;

        m_current = static_cast<double>(m_current_n) * 1000.0 / static_cast<double>(T);

        m_history[m_index] = m_current;
        m_index = (m_index + 1) % N;
        if (m_index == 0) {
            m_full = true;
        }
        if (!m_full) {
            m_mean = std::accumulate(m_history.begin(), m_history.begin()+m_index, 0.0) / m_index;
            m_variance = 1.0 / ( m_index - 1 ) * std::inner_product(m_history.begin(), m_history.begin()+m_index, m_history.begin(), 0.0,
                                               [](double const & x, double const & y) { return x + y; },
                                               [this](double const & x, double const & y) { return (x - m_mean)*(y - m_mean); });
        } else {
            m_mean = std::accumulate(m_history.begin(), m_history.end(), 0.0) / N;
            m_variance = 1.0 / ( N - 1.0 ) * std::inner_product(m_history.begin(), m_history.end(), m_history.begin(), 0.0,
                                               [](double const & x, double const & y) { return x + y; },
                                               [this](double const & x, double const & y) { return (x - m_mean)*(y - m_mean); });
        }
        m_deviation = std::sqrt( m_variance );

        m_current_n = 0;
        return true;
    }
    return false;
}

template <std::size_t N, std::size_t T>
auto RateMeasurement<N, T>::current() const noexcept -> double
{
    return m_current;
}

template <std::size_t N, std::size_t T>
auto RateMeasurement<N, T>::mean() const noexcept -> double
{
    return m_mean;
}

template <std::size_t N, std::size_t T>
auto RateMeasurement<N, T>::deviation() const noexcept -> double
{
    return m_deviation;
}

class GUID
{
public:
    GUID(std::size_t hash, std::uint64_t time);

    [[nodiscard]] auto to_string() const -> std::string;

private:
    [[nodiscard]] static auto get_mac() -> std::uint64_t;
    [[nodiscard]] static auto get_number() -> std::uint64_t;

    std::uint64_t m_first { 0 };
    std::uint64_t m_second { 0 };
};

}
#endif // UTILITY_H
