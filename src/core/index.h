#pragma once

#include "core/types.h"
#include <cstddef>
#include <format>
#include <functional>
namespace z {
template <typename Tag> struct Index {
    u32 id;

    explicit constexpr Index(u32 id) : id(id) {}

    explicit operator u32() const { return id; }
    bool operator==(const Index&) const = default;
    auto operator<=>(const Index&) const = default;
};

}; // namespace z

// NOLINTBEGIN(cert-dcl58-cpp)
namespace std {
template <typename T> struct hash<z::Index<T>> {
    size_t operator()(const z::Index<T>& id) const {
        return hash<u32>{}(id.id);
    }
};

template <typename T> struct formatter<z::Index<T>> {
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(z::Index<T> s, FmtContext& ctx) const {
        return std::format_to(ctx.out(), "({})", s.id);
    }
};
// NOLINTEND(cert-dcl58-cpp)
}; // namespace std
