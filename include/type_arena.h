#pragma once

#include "type_ref.h"
#include <cassert>
#include <memory>
#include <vector>

namespace z::type {
class Type;

class TypeArena {
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
        auto t = types.size();
        types.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return TypeRef(t);
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
