#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

template <class T, class = void>
struct is_iterator : std::false_type {};

template <class T>
struct is_iterator<T, std::void_t<typename std::iterator_traits<T>::iterator_category>>
    : std::true_type {};

enum class endian : bool { big, little };

template <typename T, endian Endian = endian::little, typename It,
          std::enable_if_t<std::is_integral<T>::value, bool> = true,
          std::enable_if_t<is_iterator<It>::value, bool> = true>
[[nodiscard]] auto get(const It& start) -> T {
    const auto& end{start + sizeof(T)};
    T value{0};
    std::size_t shift{(Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8};
    for (auto it = start; it != end; it++) {
        value += static_cast<T>(*it) << shift;
        if (Endian == endian::little) {
            shift += 8;
        } else {
            shift -= 8;
        }
    }
    return value;
}

template <typename T, endian Endian = endian::little, typename It,
          std::enable_if_t<std::is_integral<T>::value, bool> = true,
          std::enable_if_t<is_iterator<It>::value, bool> = true>
void put(const It& start, const T& value) {
    const auto& end{start + sizeof(T)};
    std::size_t shift{(Endian == endian::little) ? 0 : (sizeof(T) - 1) * 8};
    for (auto it = start; it != end; it++) {
        *it = static_cast<std::uint8_t>((value >> shift) & 0xff);
        if (Endian == endian::little) {
            shift += 8;
        } else {
            shift -= 8;
        }
    }
}

#endif // HELPER_FUNCTIONS_H