#pragma once

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
        std::cerr << std::format(
            "{}:{} panic: {}\n", fmt.loc.file_name(), fmt.loc.line(),
            std::format(fmt.fmt, std::forward<Args>(args)...));
    } catch (...) {
        std::cerr << fmt.loc.file_name() << ":" << fmt.loc.line()
                  << " panic: formatting failed\n";
    }

#ifdef ENABLE_STACKTRACE
    std::cerr << std::stacktrace::current() << '\n';
#endif
    std::cerr.flush();
    std::abort();
}

template <typename... Args>
static void constexpr expect(bool cond,
                             PanicFormat<std::type_identity_t<Args>...> fmt,
                             Args&&... args) noexcept {
    if (!cond) [[unlikely]]
        panic(fmt, std::forward<Args>(args)...);
}
