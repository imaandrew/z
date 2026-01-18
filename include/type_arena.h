#pragma once

#include "type_ref.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace z::type {
class Type;

class TypeArena {
    std::unordered_map<TypeKey, TypeRef> intern_map;
    std::unordered_set<TypeRef> interned_types;
    std::vector<std::unique_ptr<Type>> types;

public:
    static constexpr TypeRef UNINITALIZED{0};
    static constexpr TypeRef I8{1};
    static constexpr TypeRef I16{2};
    static constexpr TypeRef I32{3};
    static constexpr TypeRef I64{4};
    static constexpr TypeRef U8{5};
    static constexpr TypeRef U16{6};
    static constexpr TypeRef U32{7};
    static constexpr TypeRef U64{8};
    static constexpr TypeRef F32{9};
    static constexpr TypeRef F64{10};
    static constexpr TypeRef BOOL{11};
    static constexpr TypeRef STR{12};
    static constexpr TypeRef CHAR{13};
    static constexpr TypeRef VOID{14};
    static constexpr TypeRef INVALID{15};

    TypeArena();

    template <class T, typename... Args> TypeRef make(Args&&... args) {
        if constexpr (T::Kind == TypeKind::Inferred ||
                      T::Kind == TypeKind::Unknown ||
                      T::Kind == TypeKind::Temp) {
            TypeRef ref{static_cast<std::uint32_t>(types.size())};
            types.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            return ref;
        }

        TypeKey key = T::make_key(args...);
        if (auto it = intern_map.find(key); it != intern_map.end()) {
            return it->second;
        }

        auto t = TypeRef(types.size());
        types.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        intern_map.emplace(key, t);
        return t;
    }

    bool is_interned(TypeRef ref) const { return interned_types.contains(ref); }

    template <class T, typename... Args>
    void replace(TypeRef ref, Args&&... args) {
        assert(!is_interned(ref) && "Cannot replace an interned type!");
        types[ref.id] = std::make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] Type* get(TypeRef ref) const {
        assert(ref.is_valid() && "Tried to get invalid TypeRef");

        return types[ref.id].get();
    }

    template <typename T> T* get_as(TypeRef ref) const {
        return dyn_cast<T>(get(ref));
    }
};
} // namespace z::type
