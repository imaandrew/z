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
    TypeArena();

    template <class T, typename... Args> TypeRef make(Args&&... args) {
        if constexpr (T::Kind == TypeKind::Inferred ||
                      T::Kind == TypeKind::Unknown ||
                      T::Kind == TypeKind::Temp ||
                      T::Kind == TypeKind::Struct ||
                      T::Kind == TypeKind::Enum) {
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
        types[ref.get_id()] = std::make_unique<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] Type* get(TypeRef ref) const {
        return types[ref.get_id()].get();
    }

    template <typename T> T* get_as(TypeRef ref) const {
        return dyn_cast<T>(get(ref));
    }
};
} // namespace z::type
