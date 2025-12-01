#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z::ast {
struct Expr;
struct Identifier;
} // namespace z::ast

namespace z::type {

using TypeID = std::uint32_t;

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
    Void,
    Invalid,
    Inferred
};

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

    [[nodiscard]] virtual bool is_arithmetic_compatible(Type* /*other*/) const {
        return false;
    }

    [[nodiscard]] virtual bool
    is_assignment_compatible(const Type* other) const {
        return get_kind() == other->get_kind();
    }

    [[nodiscard]] virtual bool is_logical() const { return false; }

    [[nodiscard]] virtual bool is_comparable(Type* other) const {
        return get_kind() == other->get_kind();
    }

    [[nodiscard]] virtual bool is_integral() const { return false; }
    [[nodiscard]] virtual bool is_float() const { return false; }
    [[nodiscard]] virtual bool is_numeric() const { return false; }

    [[nodiscard]] virtual bool is_unknown() const { return false; }

    [[nodiscard]] virtual bool is_explicit() const { return true; }

    [[nodiscard]] virtual bool is_void() const { return false; }

    [[nodiscard]] virtual bool is_array() const { return false; }

    [[nodiscard]] virtual bool is_iterable() const { return false; }

    [[nodiscard]] virtual bool is_struct() const { return false; }

    virtual void dump(std::ostream& stream = std::cout) const = 0;
    [[nodiscard]] virtual std::string basic_name() const = 0;
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

public:
    static constexpr TypeKind Kind = TypeKind::Integer;

    IntegerType(const int bit_width, const bool is_signed)
        : Type(Kind), bit_width(bit_width), _signed(is_signed) {};

    [[nodiscard]] int get_width() const { return bit_width; }

    [[nodiscard]] bool is_signed() const { return _signed; }

    bool is_arithmetic_compatible(Type* other) const override {
        if (get_kind() == other->get_kind()) {
            const auto* other_int = cast<IntegerType>(other);
            return this->bit_width == other_int->bit_width &&
                   this->_signed == other_int->_signed;
        }

        return false;
    }

    [[nodiscard]] bool is_integral() const override { return true; }

    [[nodiscard]] bool is_numeric() const override { return true; }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_int = cast<const IntegerType>(other);
            return this->bit_width == other_int->bit_width &&
                   this->_signed == other_int->_signed;
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "IntegerType { bit_width: " << bit_width
               << ", is_signed: " << _signed << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        return std::format("{}{}", _signed ? "i" : "u", bit_width);
    }
};

class FloatType final : public Type {
    int bit_width;

public:
    static constexpr TypeKind Kind = TypeKind::Float;

    explicit FloatType(const int bit_width)
        : Type(Kind), bit_width(bit_width) {};

    [[nodiscard]] int get_width() const { return bit_width; }

    bool is_arithmetic_compatible(Type* other) const override {
        if (Type::is_arithmetic_compatible(other)) {
            const auto* other_float = cast<FloatType>(other);
            return this->bit_width == other_float->bit_width;
        }

        return false;
    }

    [[nodiscard]] bool is_float() const override { return true; }

    [[nodiscard]] bool is_numeric() const override { return true; }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_float = cast<const FloatType>(other);
            return this->bit_width == other_float->bit_width;
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "FloatType { bit_width: " << bit_width << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        return std::format("f{}", bit_width);
    }
};

class BooleanType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Boolean;

    BooleanType() : Type(Kind) {};

    [[nodiscard]] bool is_logical() const override { return true; }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "BooleanType";
    }

    [[nodiscard]] std::string basic_name() const override { return "bool"; }
};

class StringType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::String;

    StringType() : Type(Kind) {};

    void dump(std::ostream& stream = std::cout) const override {
        stream << "StringType";
    }

    [[nodiscard]] bool is_iterable() const override { return true; }

    [[nodiscard]] std::string basic_name() const override { return "string"; }
};

class CharType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Char;

    CharType() : Type(Kind) {};

    void dump(std::ostream& stream = std::cout) const override {
        stream << "CharType";
    }

    [[nodiscard]] std::string basic_name() const override { return "char"; }
};

class PointerType final : public Type {
    std::unique_ptr<Type> type;

public:
    static constexpr TypeKind Kind = TypeKind::Pointer;

    explicit PointerType(std::unique_ptr<Type> type)
        : Type(Kind), type(std::move(type)) {};

    [[nodiscard]] bool is_logical() const override { return true; }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* ptr = cast<const PointerType>(other);
            return type->is_assignment_compatible(ptr->type.get());
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "PointerType { type: ";
        type->dump(stream);
        stream << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        return type->basic_name() + "*";
    }
};

class ArrayType final : public Type {
    std::shared_ptr<Type> type;
    std::shared_ptr<ast::Expr> size;

public:
    static constexpr TypeKind Kind = TypeKind::Array;

    explicit ArrayType(std::shared_ptr<Type> type);
    ArrayType(std::shared_ptr<Type> type, std::shared_ptr<ast::Expr> size);
    ~ArrayType() override;

    ArrayType(const ArrayType&) = delete;
    ArrayType& operator=(const ArrayType&) = delete;
    ArrayType(ArrayType&&) = delete;
    ArrayType& operator=(ArrayType&&) = delete;

    [[nodiscard]] std::shared_ptr<Type> get_type() const { return type; }

    [[nodiscard]] bool is_array() const override { return true; }

    [[nodiscard]] bool is_iterable() const override { return true; }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            if (const auto* other_array = cast<const ArrayType>(other);
                !type->is_assignment_compatible(other_array->type.get())) {
                return false;
            }

            // TODO: return true if sizes are equal
            return false;
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "ArrayType { type: ";
        type->dump(stream);
        stream << ", size: ";
        // TODO
        stream << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        return std::format("[{}]", type->basic_name());
    }
};

class UnknownType final : public Type {
    std::unique_ptr<ast::Identifier> ident;

public:
    static constexpr TypeKind Kind = TypeKind::Unknown;

    explicit UnknownType(std::unique_ptr<ast::Identifier> ident);
    ~UnknownType() override;

    UnknownType(const UnknownType&) = delete;
    UnknownType& operator=(const UnknownType&) = delete;
    UnknownType(UnknownType&&) = delete;
    UnknownType& operator=(UnknownType&&) = delete;

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] const ast::Identifier* get_ident() const {
        return ident.get();
    }

    bool is_assignment_compatible(const Type* /*other*/) const override {
        return false;
    }

    [[nodiscard]] bool is_unknown() const override { return true; }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "UnknownType { ident: }";
    }

    [[nodiscard]] std::string basic_name() const override { return "unk"; }
};

class FunctionType final : public Type {
    std::vector<std::shared_ptr<Type>> params;
    std::shared_ptr<Type> return_val;

public:
    static constexpr TypeKind Kind = TypeKind::Function;

    explicit FunctionType(const std::vector<std::shared_ptr<Type>>& params,
                          const std::shared_ptr<Type>& return_val)
        : Type(Kind), params(params), return_val(return_val) {};

    bool is_assignment_compatible(const Type* /*other*/) const override {
        return false;
    }
    void set_return_val(const std::shared_ptr<Type>& ret) { return_val = ret; }

    [[nodiscard]] std::shared_ptr<Type> get_return_val() const {
        return return_val;
    }

    [[nodiscard]] std::vector<std::shared_ptr<Type>>& get_params() {
        return params;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "FunctionType { params: [";
        if (!params.empty()) {
            for (size_t i = 0; i < params.size() - 1; i++) {
                params[i]->dump(stream);
                stream << ", ";
            }
            params.back()->dump(stream);
        }

        stream << "], return_val: ";
        if (return_val) {
            return_val->dump(stream);
        }
        stream << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        std::string s = "(";
        for (size_t i = 0; i < params.size() - 1; i++) {
            s += params[i]->basic_name() + ", ";
        }
        s += params.back()->basic_name() + ")";

        if (return_val) {
            s += "(" + return_val->basic_name() + ")";
        } else {
            s += "()";
        }

        return s;
    }
};

class StructType final : public Type {
    std::string name;
    std::unordered_map<std::string_view, std::shared_ptr<Type>> fields;

public:
    static constexpr TypeKind Kind = TypeKind::Struct;

    explicit StructType(const std::unique_ptr<ast::Identifier>& ident);

    [[nodiscard]] bool is_struct() const override { return true; }

    bool define_field(std::string_view field,
                      const std::shared_ptr<Type>& type) {
        return fields.insert({field, type}).second;
    }

    void replace_field_type(std::string_view field,
                            const std::shared_ptr<Type>& type) {
        fields.insert_or_assign(field, type);
    }

    std::shared_ptr<Type> get_field_type(std::string_view field) const {
        if (!fields.contains(field))
            return nullptr;

        return fields.at(field);
    }

    const std::unordered_map<std::string_view, std::shared_ptr<Type>>&
    get_fields() const {
        return fields;
    }

    bool has_field(std::string_view field) const {
        return fields.contains(field);
    }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_struct = cast<const StructType>(other);
            return name == other_struct->basic_name();
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "StructType { name: " << name << " }";
    }

    [[nodiscard]] std::string basic_name() const override { return name; }
};

class EnumType final : public Type {
    std::string name;
    std::unordered_map<std::string_view, std::vector<std::shared_ptr<Type>>&>
        fields;

public:
    static constexpr TypeKind Kind = TypeKind::Enum;

    explicit EnumType(const std::unique_ptr<ast::Identifier>& ident);

    bool define_field(std::string_view field,
                      std::vector<std::shared_ptr<Type>>& types) {
        return fields.insert({field, types}).second;
    }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_enum = cast<const EnumType>(other);
            return name == other_enum->basic_name();
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "EnumType { name: " << name << " }";
    }

    [[nodiscard]] std::string basic_name() const override { return name; }
};

class EnumVariantType final : public Type {
    std::string parent_enum;

public:
    static constexpr TypeKind Kind = TypeKind::EnumVariant;

    explicit EnumVariantType(std::string parent_enum)
        : Type(Kind), parent_enum(std::move(parent_enum)) {};

    [[nodiscard]] const std::string& get_parent_enum() const {
        return parent_enum;
    }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_enum = cast<const EnumVariantType>(other);
            return parent_enum == other_enum->get_parent_enum();
        }

        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "EnumVariantType { parent: " << parent_enum << " }";
    }

    [[nodiscard]] std::string basic_name() const override {
        return parent_enum;
    }
};

class VoidType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Void;

    VoidType() : Type(Kind) {};

    [[nodiscard]] bool is_void() const override { return true; }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "VoidType";
    }

    [[nodiscard]] std::string basic_name() const override { return "()"; }
};

class InvalidType final : public Type {
public:
    static constexpr TypeKind Kind = TypeKind::Invalid;

    InvalidType() : Type(Kind) {};

    bool is_assignment_compatible(const Type* /*other*/) const override {
        return false;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "InvalidType";
    }

    [[nodiscard]] std::string basic_name() const override {
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

    [[nodiscard]] TypeID get_id() const { return id; }

    [[nodiscard]] bool is_explicit() const override { return false; }

    [[nodiscard]] InferType get_infer_type() const { return infer_type; }

    bool is_assignment_compatible(const Type* other) const override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_type = cast<const InferredType>(other);
            if (infer_type == InferType::Var ||
                other_type->infer_type == InferType::Var ||
                infer_type == InferType::Block ||
                other_type->infer_type == InferType::Block) {
                return true;
            }

            return infer_type == other_type->infer_type;
        }

        if (other->is_integral() && infer_type == InferType::IntLiteral) {
            return true;
        }

        if (other->is_float() && infer_type == InferType::FloatLiteral) {
            return true;
        }

        return infer_type == InferType::Var || infer_type == InferType::Block;
    }

    void dump(std::ostream& stream = std::cout) const override {
        stream << "InferrableType";
    }

    [[nodiscard]] std::string basic_name() const override {
        switch (infer_type) {
        case InferType::IntLiteral:
            return "integer";
        case InferType::FloatLiteral:
            return "float";
        default:
            return "unknown";
        }
    }
};
} // namespace z::type
