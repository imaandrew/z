#pragma once

#include <cstdint>

namespace z::type {
class TypeRef {
    std::uint32_t id = 0;
    constexpr explicit TypeRef(std::uint32_t id) : id(id) {}
    friend class TypeArena;

public:
    TypeRef() = default;
    [[nodiscard]] bool is_valid() const { return id != 0; }

    bool operator==(const TypeRef& other) const { return id == other.id; }
};
} // namespace z::type
