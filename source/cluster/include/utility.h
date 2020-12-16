#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <vector>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace MuonPi {

class MessageConstructor
{
public:
    MessageConstructor(char delimiter);

    void add_field(const std::string& field);

    [[nodiscard]] auto get_string() const -> std::string;

private:
    std::string m_message {};
    char m_delimiter;
};


class MessageParser
{
public:
    MessageParser(const std::string& message, char delimiter);

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto operator[](std::size_t i) const -> std::string;

private:
    void skip_delimiter();
    void read_field();
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
    void increase_counter();

    auto step() -> bool;

    [[nodiscard]] auto current() const noexcept -> double;

    [[nodiscard]] auto mean() const noexcept -> double;

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

}
#endif // UTILITY_H
