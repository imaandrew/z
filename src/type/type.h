#pragma once

#include "core/string_pool.h"
#include "diag/src_mgr.h"
#include "type_arena.h"
#include "type_ref.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ast {
struct Expr;
struct Identifier;
} // namespace z::ast

namespace z {
struct ZContext;
}

namespace z::type {

using TypeID = std::uint32_t;

class Type {
    const TypeKind kind;

public:
    explicit Type(TypeKind k) : kind(k) {};
    virtual ~Type() = default;

    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;
    Type(Type&&) = delete;
    Type& operator=(Type&&) = delete;

    [[nodiscard]] TypeKind get_kind() const { return kind; }

    virtual bool operator==(const Type& other) const {
        return kind == other.kind;
    };

    [[nodiscard]] virtual bool is_logical() const { return false; }

    [[nodiscard]] virtual bool is_integral() const { return false; }
    [[nodiscard]] virtual bool is_float() const { return false; }
    [[nodiscard]] virtual bool is_numeric() const { return false; }

    [[nodiscard]] virtual bool is_unknown() const { return false; }

    [[nodiscard]] virtual bool is_explicit() const { return true; }

    [[nodiscard]] virtual bool is_void() const { return false; }

    [[nodiscard]] virtual bool is_array() const { return false; }

    [[nodiscard]] virtual bool is_iterable() const { return false; }

    [[nodiscard]] virtual bool is_struct() const { return false; }

    virtual void dump(ZContext* ctxt,
                      std::ostream& stream = std::cout) const = 0;
    [[nodiscard]] virtual std::string
    basic_name(const ZContext* ctxt) const = 0;
};

template <typename T> bool isa(const Type* type) {
    return type->get_kind() == T::Kind;
}

template <typename T> T* dyn_cast(Type* type) {
    if (isa<T>(type))
        return static_cast<T*>(type);
    return nullptr;
}

template <typename T> const T* dyn_cast(const Type* type) {
    if (isa<T>(type))
        return static_cast<const T*>(type);
    return nullptr;
}

template <typename T> T* cast(Type* type) {
    assert(isa<T>(type) && "Invalid cast");
    return static_cast<T*>(type);
}

template <typename T> const T* cast(const Type* type) {
    assert(isa<T>(type) && "Invalid cast");
    return static_cast<T*>(type);
}

class IntegerType final : public Type {
    int bit_width;
    bool _signed;
    bool _size_type = false;

public:
    static constexpr TypeKind Kind = TypeKind::Integer;

    IntegerType(const int bit_width, const bool is_signed)
        : Type(Kind), bit_width(bit_width), _signed(is_signed) {}

    explicit IntegerType(const bool is_signed)
        : Type(Kind), bit_width(sizeof(std::size_t)), _signed(is_signed),
          _size_type(true) {}

    static TypeKey make_key(const int bit_width, const bool is_signed,
                            const bool size_type) {
        return make_type_key(Kind, bit_width, is_signed, size_type);
    }

    [[nodiscard]] int get_width() const { return bit_width; }

    [[nodiscard]] bool is_signed() const { return _signed; }

    [[nodiscard]] bool is_size_type() const { return _size_type; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<IntegerType>(&other)) {
            return bit_width == other_->bit_width &&
                   _signed == other_->_signed &&
                   _size_type == other_->_size_type;
        }

        return false;
    }

    [[nodiscard]] bool is_integral() const override { return true; }

    [[nodiscard]] bool is_numeric() const override { return true; }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        std::print(stream,
                   "IntegerType {{ bit_width: {}, is_signed: {}, "
                   "is_size_type: {} }}",
                   bit_width, _signed, _size_type);
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        if (_size_type)
            return std::format("{}size", _signed ? "i" : "u");
        return std::format("{}{}", _signed ? "i" : "u", bit_width);
    }
};

class FloatType final : public Type {
    int bit_width;

public:
    static constexpr TypeKind Kind = TypeKind::Float;

    explicit FloatType(const int bit_width)
        : Type(Kind), bit_width(bit_width) {};

    static TypeKey make_key(const int bit_width) {
        return make_type_key(Kind, bit_width);
    }

    [[nodiscard]] int get_width() const { return bit_width; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<FloatType>(&other)) {
            return bit_width == other_->bit_width;
        }

        return false;
    }

    [[nodiscard]] bool is_float() const override { return true; }

    [[nodiscard]] bool is_numeric() const override { return true; }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        std::print(stream, "FloatType {{ bit_width: {} }}", bit_width);
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return std::format("f{}", bit_width);
    }
};

class BooleanType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Boolean;

    BooleanType() : Type(Kind) {};

    static TypeKey make_key() { return make_type_key(Kind); }

    [[nodiscard]] bool is_logical() const override { return true; }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        stream << "BooleanType";
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "bool";
    }
};

class StringType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::String;

    StringType() : Type(Kind) {};

    static TypeKey make_key() { return make_type_key(Kind); }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        stream << "StringType";
    }

    [[nodiscard]] bool is_iterable() const override { return true; }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "string";
    }
};

class CharType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Char;

    CharType() : Type(Kind) {};

    static TypeKey make_key() { return make_type_key(Kind); }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        stream << "CharType";
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "char";
    }
};

class PointerType final : public Type {
    TypeRef type;

public:
    static constexpr TypeKind Kind = TypeKind::Pointer;

    explicit PointerType(TypeRef type) : Type(Kind), type(type) {};

    static TypeKey make_key(TypeRef type) { return make_type_key(Kind, type); }

    [[nodiscard]] bool is_logical() const override { return true; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<PointerType>(&other))
            return type == other_->type;

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class ArrayType final : public Type {
    TypeRef type;
    std::optional<std::uint64_t> size;

public:
    static constexpr TypeKind Kind = TypeKind::Array;

    ArrayType(TypeRef type, std::optional<std::uint64_t> size)
        : Type(Kind), type(type), size(size) {};

    static TypeKey make_key(TypeRef type, std::optional<std::uint64_t> size) {
        return make_type_key(Kind, type, size.has_value(), size.value_or(0));
    }

    [[nodiscard]] TypeRef get_type() const { return type; }

    [[nodiscard]] std::optional<std::uint64_t> get_size() const { return size; }

    [[nodiscard]] bool is_array() const override { return true; }

    [[nodiscard]] bool is_iterable() const override { return true; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<ArrayType>(&other)) {
            if (type != other_->type)
                return false;

            if (size && other_->size)
                return size == other_->size;

            return !size && !other_->size;
        }

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class UnknownType final : public Type {
    StringID ident;
    Span span;

public:
    static constexpr TypeKind Kind = TypeKind::Unknown;

    UnknownType(StringID ident, Span span)
        : Type(Kind), ident(ident), span(span) {}

    static TypeKey make_key(StringID ident, Span span) {
        return make_type_key(Kind, ident, span);
    }

    [[nodiscard]] StringID get_id() const { return ident; }

    [[nodiscard]] Span get_span() const { return span; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<UnknownType>(&other)) {
            return ident == other_->ident;
        }

        return false;
    }

    [[nodiscard]] bool is_unknown() const override { return true; }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "unk";
    }
};

class FunctionType final : public Type {
    std::vector<TypeRef> params;
    TypeRef return_val;

public:
    static constexpr TypeKind Kind = TypeKind::Function;

    explicit FunctionType(std::vector<TypeRef> params, TypeRef return_val)
        : Type(Kind), params(std::move(params)), return_val(return_val) {};

    static TypeKey make_key(const std::vector<TypeRef>& params,
                            TypeRef return_val) {
        std::size_t params_hash = params.size();
        for (const auto& p : params) {
            params_hash ^= p.get_id() + 0x9e3779b9 + (params_hash << 6U) +
                           (params_hash >> 2U);
        }
        return make_type_key(Kind, return_val, params_hash);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<FunctionType>(&other)) {
            if (params.size() != other_->params.size())
                return false;

            for (std::size_t i = 0; i < params.size(); i++) {
                if (params[i] != other_->params[i])
                    return false;
            }

            return return_val == other_->return_val;
        }

        return false;
    }

    [[nodiscard]] TypeRef get_return_val() const { return return_val; }

    [[nodiscard]] const std::vector<TypeRef>& get_params() const {
        return params;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class StructType final : public Type {
    StringID name;
    std::unordered_map<StringID, std::pair<TypeRef, std::uint32_t>> fields;
    std::unordered_map<StringID, std::pair<TypeRef, std::uint32_t>> funcs;

public:
    static constexpr TypeKind Kind = TypeKind::Struct;

    explicit StructType(
        StringID name,
        std::unordered_map<StringID, std::pair<TypeRef, std::uint32_t>> fields,
        std::unordered_map<StringID, std::pair<TypeRef, std::uint32_t>> funcs)
        : Type(Kind), name(name), fields(std::move(fields)),
          funcs(std::move(funcs)) {}

    static TypeKey
    make_key(StringID name, const std::unordered_map<StringID, TypeRef>& fields,
             const std::unordered_map<StringID, TypeRef>& funcs) {
        std::size_t fields_hash = fields.size();
        for (const auto& [k, v] : fields) {
            fields_hash ^=
                k.id + 0x9e3779b9 + (fields_hash << 6U) + (fields_hash >> 2U);
            fields_hash ^= v.get_id() + 0x9e3779b9 + (fields_hash << 6U) +
                           (fields_hash >> 2U);
        }

        std::size_t funcs_hash = funcs.size();
        for (const auto& [k, v] : funcs) {
            funcs_hash ^=
                k.id + 0x9e3779b9 + (funcs_hash << 6U) + (funcs_hash >> 2U);
            funcs_hash ^= v.get_id() + 0x9e3779b9 + (funcs_hash << 6U) +
                          (funcs_hash >> 2U);
        }
        return make_type_key(Kind, name, fields_hash, funcs_hash);
    }

    [[nodiscard]] bool is_struct() const override { return true; }

    StringID get_name() const { return name; }

    std::optional<TypeRef> get_field_type(StringID field) const {
        if (!fields.contains(field))
            return std::nullopt;

        return fields.at(field).first;
    }

    const std::unordered_map<StringID, std::pair<TypeRef, std::uint32_t>>&
    get_fields() const {
        return fields;
    }

    bool has_field(StringID field) const { return fields.contains(field); }

    std::optional<std::uint32_t> get_field_index(StringID field) const {
        if (!fields.contains(field))
            return std::nullopt;

        return fields.at(field).second;
    }

    std::optional<TypeRef> get_func_type(StringID func) const {
        if (!funcs.contains(func))
            return std::nullopt;

        return funcs.at(func).first;
    }

    std::optional<std::uint32_t> get_func_index(StringID field) const {
        if (!funcs.contains(field))
            return std::nullopt;

        return funcs.at(field).second;
    }

    bool operator==(const Type& other) const override {
        const auto* other_ = dyn_cast<StructType>(&other);
        if (!other_)
            return false;

        if (name != other_->name)
            return false;

        if (fields.size() != other_->fields.size())
            return false;

        if (funcs.size() != other_->funcs.size())
            return false;

        for (const auto& [field, type] : fields) {
            if (const auto f = other_->get_field_type(field);
                f && *f != type.first)
                return false;
        }

        return std::ranges::all_of(
            funcs.begin(), funcs.end(), [&other_](const auto& f) {
                return other_->get_func_type(f.first) == f.second.first;
            });
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class EnumType final : public Type {
    StringID name;
    std::unordered_map<StringID, std::vector<TypeRef>&> fields;

public:
    static constexpr TypeKind Kind = TypeKind::Enum;

    explicit EnumType(
        StringID name,
        std::unordered_map<StringID, std::vector<TypeRef>&> fields)
        : Type(Kind), name(name), fields(std::move(fields)) {}

    static TypeKey make_key(
        StringID name,
        const std::unordered_map<StringID, std::vector<TypeRef>&>& fields) {
        std::size_t fields_hash = fields.size();
        for (const auto& [k, v] : fields) {
            fields_hash ^=
                k.id + 0x9e3779b9 + (fields_hash << 6U) + (fields_hash >> 2U);
            for (const auto& vv : v) {
                fields_hash ^= vv.get_id() + 0x9e3779b9 + (fields_hash << 6U) +
                               (fields_hash >> 2U);
            }
        }
        return make_type_key(Kind, name, fields_hash);
    }

    bool operator==(const Type& other) const override {
        const auto* other_ = dyn_cast<EnumType>(&other);
        if (!other_)
            return false;

        if (name != other_->name)
            return false;

        if (fields.size() != other_->fields.size())
            return false;

        for (const auto& [field, types] : fields) {
            if (!other_->fields.contains(field))
                return false;

            const auto other_types = fields.at(field);
            if (types.size() != other_types.size())
                return false;

            for (std::size_t i = 0; i < types.size(); i++) {
                if (types[i] != other_types[i])
                    return false;
            }
        }

        return true;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class EnumVariantType final : public Type {
    StringID parent_enum;

public:
    static constexpr TypeKind Kind = TypeKind::EnumVariant;

    explicit EnumVariantType(StringID parent_enum)
        : Type(Kind), parent_enum(parent_enum) {};

    static TypeKey make_key(StringID parent_enum) {
        return make_type_key(Kind, parent_enum);
    }

    [[nodiscard]] StringID get_parent_enum() const { return parent_enum; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<EnumVariantType>(&other))
            return parent_enum == other_->parent_enum;

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class TupleType final : public Type {
    TypeRef first;
    TypeRef second;

public:
    static constexpr TypeKind Kind = TypeKind::Tuple;

    TupleType(TypeRef first, TypeRef second)
        : Type(Kind), first(first), second(second) {};

    static TypeKey make_key(TypeRef first, TypeRef second) {
        return make_type_key(Kind, first, second);
    }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<TupleType>(&other)) {
            return first == other_->first && second == other_->second;
        }

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class VoidType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Void;

    VoidType() : Type(Kind) {}

    static TypeKey make_key() { return make_type_key(Kind); }

    [[nodiscard]] bool is_void() const override { return true; }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        stream << "VoidType";
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "()";
    }
};

class InvalidType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Invalid;

    InvalidType() : Type(Kind) {};

    static TypeKey make_key() { return make_type_key(Kind); }

    bool operator==(const Type& /* other*/) const override { return false; }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        stream << "InvalidType";
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return "invalid_type";
    }
};

enum class InferType : std::uint8_t { IntLiteral, FloatLiteral, Var, Block };

class InferredType final : public Type {
    TypeID id;
    InferType infer_type;

public:
    static constexpr TypeKind Kind = TypeKind::Inferred;

    explicit InferredType(TypeID id, InferType infer_type = InferType::Var)
        : Type(Kind), id(id), infer_type(infer_type) {};

    static TypeKey make_key(TypeID id, InferType infer_type = InferType::Var) {
        return make_type_key(Kind, id, infer_type);
    }

    [[nodiscard]] TypeID get_id() const { return id; }

    [[nodiscard]] bool is_explicit() const override { return false; }

    [[nodiscard]] InferType get_infer_type() const { return infer_type; }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<InferredType>(&other))
            return id == other_->id && infer_type == other_->infer_type;

        return false;
    }

    void dump(ZContext* /*ctxt*/,
              std::ostream& stream = std::cout) const override {
        constexpr auto to_string = [](auto t) {
            switch (t) {
            case InferType::IntLiteral:
                return "integer";
            case InferType::FloatLiteral:
                return "float";
            case InferType::Block:
                return "block";
            case InferType::Var:
                return "var";
            }
            return "unknown";
        };

        std::print(stream, "InferrableType {{ id: {}, infer_type: {} }}", id,
                   to_string(infer_type));
    }

    [[nodiscard]] std::string
    basic_name(const ZContext* /*ctxt*/) const override {
        return std::format("?{}", id);
    }
};

class TypeType final : public Type {
    TypeRef internal_type;

public:
    static constexpr TypeKind Kind = TypeKind::Type;

    explicit TypeType(TypeRef type) : Type(Kind), internal_type(type) {}

    static TypeKey make_key(TypeRef type) { return make_type_key(Kind, type); }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<TypeType>(&other))
            return internal_type == other_->internal_type;

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class TraitType final : public Type {
    StringID name;

public:
    static constexpr TypeKind Kind = TypeKind::Trait;

    explicit TraitType(StringID name) : Type(Kind), name(name) {}

    static TypeKey make_key(StringID name) { return make_type_key(Kind, name); }

    bool operator==(const Type& other) const override {
        if (const auto* other_ = dyn_cast<TraitType>(&other))
            return name == other_->name;

        return false;
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};

class TempType final : public Type {
    StringID name;
    TypeKind real_type;

public:
    static constexpr TypeKind Kind = TypeKind::Temp;

    TempType(StringID name, TypeKind real_type)
        : Type(Kind), name(name), real_type(real_type) {}

    static TypeKey make_key(StringID name, TypeKind real_type) {
        return make_type_key(Kind, name, real_type);
    }

    void dump(ZContext* ctxt, std::ostream& stream = std::cout) const override;

    [[nodiscard]] std::string basic_name(const ZContext* ctxt) const override;
};
} // namespace z::type
