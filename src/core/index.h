#pragma once

#include "core/types.h"
#include <cstddef>
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
// NOLINTEND(cert-dcl58-cpp)
}; // namespace std
