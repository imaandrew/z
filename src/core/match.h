#pragma once

namespace z {
template <typename T, typename... Args>
constexpr bool matches(const T& val, const Args&... args) {
    return ((val == args) || ...);
}

} // namespace z
