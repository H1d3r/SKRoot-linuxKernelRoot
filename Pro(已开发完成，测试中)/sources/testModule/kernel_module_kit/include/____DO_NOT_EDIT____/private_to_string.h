#pragma once
#include <string>
#include <sstream>
#include <type_traits>
namespace kernel_module {
namespace detail {
// 通用类型转 string
template<typename T>
std::string to_string_generic(const T &value) {
    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, const char *>) {
        return std::string(value);
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
}

// string -> 通用类型
inline std::string trim_copy(std::string_view sv) {
    auto l = sv.begin(), r = sv.end();
    while (l != r && std::isspace(static_cast<unsigned char>(*l))) ++l;
    while (l != r && std::isspace(static_cast<unsigned char>(*(r-1)))) --r;
    return std::string(l, r);
}
template<typename T>
T from_string_generic(std::string_view sv, int base = 0) {
    if constexpr (std::is_same_v<T, std::string>) {
        return std::string(sv);
    } else if constexpr (std::is_same_v<T, bool>) {
        auto s = trim_copy(sv);
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        if (s == "1" || s == "true" || s == "yes" || s == "on")  return true;
        return false;
    } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        long long v = std::stoll(std::string(sv), nullptr, base);
        return static_cast<T>(v);
    } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
        unsigned long long v = std::stoull(std::string(sv), nullptr, base);
        return static_cast<T>(v);
    } else if constexpr (std::is_floating_point_v<T>) {
        long double v = std::stold(std::string(sv));
        return static_cast<T>(v);
    } else {
        std::istringstream iss{std::string(sv)};
        T value{};
        return value;
    }
}
}

}