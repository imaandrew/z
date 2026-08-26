#pragma once

#include "core/colour.h"
#include <cstdlib>
#include <format>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>

#ifdef ENABLE_STACKTRACE
#include <stacktrace>
#endif

template <typename T>
concept NotStringView = !std::is_convertible_v<const T&, std::string_view>;

template <typename... Args> struct PanicFormat {
    std::format_string<Args...> fmt;
    std::source_location loc;

    template <typename Str>
    explicit(NotStringView<Str>) consteval PanicFormat(
        const Str& s,
        const std::source_location& loc = std::source_location::current())
        : fmt(s), loc(loc) {}
};

template <typename... Args>
[[noreturn]] static void constexpr panic(
    PanicFormat<std::type_identity_t<Args>...> fmt, Args&&... args) noexcept {
    try {
        std::println(std::cerr, "{}:{} {}panic:{} {}", fmt.loc.file_name(),
                     fmt.loc.line(), z::colour::RED, z::colour::RESET,
                     std::format(fmt.fmt, std::forward<Args>(args)...));
    } catch (...) {
        std::println(std::cerr, "{}:{} {}panic:{} formatting failed",
                     fmt.loc.file_name(), fmt.loc.line(), z::colour::RED,
                     z::colour::RESET);
    }

#ifdef ENABLE_STACKTRACE
    std::cerr << std::stacktrace::current() << '\n';
#endif
    std::cerr.flush();
    std::abort();
}

#define ASSERT(condition)                                                      \
    (static_cast<bool>(condition)                                              \
         ? void(0)                                                             \
         : panic("assertation failed: {}", #condition))

template <typename... Args>
static void constexpr expect(bool cond,
                             PanicFormat<std::type_identity_t<Args>...> fmt,
                             Args&&... args) noexcept {
    if (!cond) [[unlikely]]
        panic(fmt, std::forward<Args>(args)...);
}
