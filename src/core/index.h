#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
namespace z {
template <typename Tag> struct Index {
    std::uint32_t id;

    explicit constexpr Index(std::uint32_t id) : id(id) {}

    explicit operator std::uint32_t() const { return id; }
    bool operator==(const Index&) const = default;
    auto operator<=>(const Index&) const = default;
};

}; // namespace z

// NOLINTBEGIN(cert-dcl58-cpp)
namespace std {
template <typename T> struct hash<z::Index<T>> {
    size_t operator()(const z::Index<T>& id) const {
        return hash<uint32_t>{}(id.id);
    }
};
// NOLINTEND(cert-dcl58-cpp)
}; // namespace std
