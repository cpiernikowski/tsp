#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <vector>
#include <concepts>
#include <ranges>
#include <optional>

namespace util {

template <typename T>
requires std::is_arithmetic_v<T>
T factorial(T t) noexcept {
    assert (t > 0);
    T out = 1;

    while (t > 1) {
        out *= t--;
    }

    return out;
}

template <typename T>
requires std::equality_comparable<T>
bool contains(const std::vector<T>& v, const T& t) {
    for (const auto& e : v) {
        if (e == t) {
            return true;
        }
    }
    return false;
}

template <template <typename> typename Container, typename T, typename F>
requires std::ranges::input_range<Container<T>>
std::optional<std::reference_wrapper<const T>> find_if_optional(const Container<T>& container, F&& predicate) {
    if (auto&& result = std::ranges::find_if(container, std::forward<F&&>(predicate));
        result != container.end()) {
        return *result;
    }

    return std::nullopt;
}

template <template <typename> typename Container, typename T, typename F>
requires std::ranges::input_range<Container<T>>
const auto& find_if_default(const Container<T>& container, F&& predicate, const T& def) {
    if (auto&& result = std::ranges::find_if(container, std::forward<F&&>(predicate));
        result != container.end()) {
        return *result;
    }
    
    return def;
}
}
#endif
