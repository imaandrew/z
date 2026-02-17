#pragma once

#include "core/string_pool.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace z::type {
enum BuiltInType : std::uint8_t {
    BT_UNINITIALIZED = 0,
    BT_I8,
    BT_I16,
    BT_I32,
    BT_I64,
    BT_U8,
    BT_U16,
    BT_U32,
    BT_U64,
    BT_ISIZE,
    BT_USIZE,
    BT_F32,
    BT_F64,
    BT_BOOL,
    BT_STR,
    BT_CHAR,
    BT_VOID,
    BT_INVALID,
    BT_COUNT
};

class TypeRef {
    std::uint32_t id = 0;

public:
    TypeRef() = default;
    constexpr explicit TypeRef(std::uint32_t id) : id(id) {}
    [[nodiscard]] bool is_initialized() const { return id != BT_UNINITIALIZED; }
    [[nodiscard]] bool is_valid() const {
        return id != BT_UNINITIALIZED && id != BT_INVALID;
    }
    [[nodiscard]] std::uint32_t get_id() const { return id; }

    bool operator==(const TypeRef& other) const { return id == other.id; }
};

namespace builtin {
inline constexpr TypeRef UNINITALIZED{BT_UNINITIALIZED};
inline constexpr TypeRef I8{BT_I8};
inline constexpr TypeRef I16{BT_I16};
inline constexpr TypeRef I32{BT_I32};
inline constexpr TypeRef I64{BT_I64};
inline constexpr TypeRef U8{BT_U8};
inline constexpr TypeRef U16{BT_U16};
inline constexpr TypeRef U32{BT_U32};
inline constexpr TypeRef U64{BT_U64};
inline constexpr TypeRef ISIZE{BT_ISIZE};
inline constexpr TypeRef USIZE{BT_USIZE};
inline constexpr TypeRef F32{BT_F32};
inline constexpr TypeRef F64{BT_F64};
inline constexpr TypeRef BOOL{BT_BOOL};
inline constexpr TypeRef STR{BT_STR};
inline constexpr TypeRef CHAR{BT_CHAR};
inline constexpr TypeRef VOID{BT_VOID};
inline constexpr TypeRef INVALID{BT_INVALID};
} // namespace builtin

enum class TypeKind : std::uint8_t {
    Integer,
    Float,
    Boolean,
    Char,
    String,
    Pointer,
    Array,
    Unknown,
    Function,
    Struct,
    Enum,
    EnumVariant,
    Tuple,
    Void,
    Invalid,
    Inferred,
    Type,
    Trait,
    Temp
};

struct TypeKey {
    TypeKind kind;
    std::array<std::uint64_t, 4> data;
    bool operator==(const TypeKey&) const = default;
};

template <typename T> constexpr std::uint64_t to_key_value(T val) {
    if constexpr (std::is_same_v<T, TypeRef>) {
        return val.get_id();
    } else if constexpr (std::is_same_v<T, StringID>) {
        return val.id;
    } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> ||
                         std::is_enum_v<T>) {
        return static_cast<std::uint64_t>(val);
    } else if constexpr (std::is_same_v<T, bool>) {
        return val ? 1 : 0;
    } else {
        static_assert(sizeof(T) <= sizeof(std::uint64_t),
                      "Type too large for key");
        return std::bit_cast<std::uint64_t>(val);
    }
}
template <typename... Args>
constexpr TypeKey make_type_key(TypeKind kind, Args... args) {
    static_assert(sizeof...(Args) <= 4, "Too many key components");
    return TypeKey{kind, {to_key_value(args)...}};
}

} // namespace z::type

template <> struct std::hash<z::type::TypeKey> {
    std::size_t operator()(const z::type::TypeKey& k) const {
        std::size_t h = std::hash<int>{}(static_cast<int>(k.kind));
        for (auto v : k.data) {
            h ^= std::hash<std::uint64_t>{}(v) + 0x9e3779b9 + (h << 6U) +
                 (h >> 2U);
        }
        return h;
    }
};

template <> struct std::hash<z::type::TypeRef> {
    size_t operator()(const z::type::TypeRef& id) const {
        return std::hash<std::uint32_t>{}(id.get_id());
    }
};
