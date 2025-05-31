#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct Expr;
struct Identifier;

class Type {
public:
    virtual ~Type() = default;
    Type() = default;

    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;
    Type(Type&&) = delete;
    Type& operator=(Type&&) = delete;

    virtual bool is_arithmetic_compatible(Type* /*other*/) { return false; }

    virtual bool is_assignable() { return false; }

    virtual bool is_assignment_compatible(Type* other) {
        return typeid(*this) == typeid(*other);
    }

    virtual bool is_logical() { return false; }

    virtual bool is_comparable(Type* other) {
        return typeid(*this) == typeid(*other);
    }

    virtual bool is_integral() { return false; }
    virtual bool is_numeric() { return false; }

    virtual bool is_unknown() { return false; }

    virtual bool is_variable() { return false; }
};

class IntegerType final : public Type {
    int bit_width;
    bool is_signed;

public:
    IntegerType(const int bit_width, const bool is_signed)
        : bit_width(bit_width), is_signed(is_signed) {};

    bool is_arithmetic_compatible(Type* other) override {
        if (typeid(*this) == typeid(*other)) {
            const auto* other_int = dynamic_cast<IntegerType*>(other);
            return this->bit_width == other_int->bit_width &&
                   this->is_signed == other_int->is_signed;
        }

        return false;
    }

    bool is_integral() override { return true; }

    bool is_numeric() override { return true; }

    bool is_assignment_compatible(Type* other) override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_int = dynamic_cast<IntegerType*>(other);
            return this->bit_width == other_int->bit_width &&
                   this->is_signed == other_int->is_signed;
        }

        return false;
    }
};

class FloatType final : public Type {
    int bit_width;

public:
    explicit FloatType(const int bit_width) : bit_width(bit_width) {};

    bool is_arithmetic_compatible(Type* other) override {
        if (Type::is_arithmetic_compatible(other)) {
            const auto* other_float = dynamic_cast<FloatType*>(other);
            return this->bit_width == other_float->bit_width;
        }

        return false;
    }

    bool is_numeric() override { return true; }

    bool is_assignment_compatible(Type* other) override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_float = dynamic_cast<FloatType*>(other);
            return this->bit_width == other_float->bit_width;
        }

        return false;
    }
};

class BooleanType final : public Type {
public:
    bool is_logical() override { return true; }
};

class StringType final : public Type {};

class CharType final : public Type {};

class PointerType final : public Type {
    std::unique_ptr<Type> type;

public:
    explicit PointerType(std::unique_ptr<Type> type) : type(std::move(type)) {};

    bool is_logical() override { return true; }

    bool is_assignment_compatible(Type* other) override {
        if (Type::is_assignment_compatible(other)) {
            const auto* ptr = dynamic_cast<PointerType*>(other);
            return type->is_assignment_compatible(ptr->type.get());
        }

        return false;
    }
};

class ArrayType final : public Type {
    std::unique_ptr<Type> type;
    std::unique_ptr<Expr> size;

public:
    explicit ArrayType(std::unique_ptr<Type> type);
    ArrayType(std::unique_ptr<Type> type, std::unique_ptr<Expr> size);
    ~ArrayType() override;

    ArrayType(const ArrayType&) = delete;
    ArrayType& operator=(const ArrayType&) = delete;
    ArrayType(ArrayType&&) = delete;
    ArrayType& operator=(ArrayType&&) = delete;

    bool is_assignment_compatible(Type* other) override {
        if (Type::is_assignment_compatible(other)) {
            if (const auto* other_array = dynamic_cast<ArrayType*>(other);
                !type->is_assignment_compatible(other_array->type.get())) {
                return false;
            }

            // TODO: return true if sizes are equal
            return false;
        }

        return false;
    }
};

class UnknownType final : public Type {
    std::unique_ptr<Identifier> ident;

public:
    explicit UnknownType(std::unique_ptr<Identifier> ident);
    ~UnknownType() override;

    UnknownType(const UnknownType&) = delete;
    UnknownType& operator=(const UnknownType&) = delete;
    UnknownType(UnknownType&&) = delete;
    UnknownType& operator=(UnknownType&&) = delete;

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] const Identifier* get_ident() const { return ident.get(); }

    bool is_assignment_compatible(Type* /*other*/) override { return false; }

    bool is_unknown() override { return true; }
};

class FunctionType final : public Type {
    std::vector<std::shared_ptr<Type>> params;
    std::shared_ptr<Type> return_val;

public:
    explicit FunctionType(const std::vector<std::shared_ptr<Type>>& params,
                          const std::shared_ptr<Type>& return_val)
        : params(params), return_val(return_val) {};

    bool is_assignment_compatible(Type* /*other*/) override { return false; }
    void set_return_val(const std::shared_ptr<Type>& ret) { return_val = ret; }
};

class StructType final : public Type {
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<Type>> fields;

public:
    explicit StructType(const std::unique_ptr<Identifier>& ident);

    bool define_field(const std::string& field,
                      const std::shared_ptr<Type>& type) {
        return fields.insert({field, type}).second;
    }

    void replace_field_type(const std::string& field,
                            const std::shared_ptr<Type>& type) {
        fields.insert_or_assign(field, type);
    }

    bool is_assignment_compatible(Type* other) override;
};

class EnumType final : public Type {
    std::string name;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Type>>&> fields;

public:
    explicit EnumType(const std::unique_ptr<Identifier>& ident);

    bool define_field(const std::string& field,
                      std::vector<std::shared_ptr<Type>>& types) {
        return fields.insert({field, types}).second;
    }

    bool is_assignment_compatible(Type* other) override;
};

class VariableType final : public Type {
    std::shared_ptr<Type> internal_type;
    bool is_const;
    bool is_static;

public:
    explicit VariableType(std::shared_ptr<Type> internal_type,
                          const bool is_const = false,
                          const bool is_static = false)
        : internal_type(std::move(internal_type)), is_const(is_const),
          is_static(is_static) {};

    void replace_type(const std::shared_ptr<Type>& type) {
        internal_type = type;
    }

    bool is_assignable() override { return true; }

    bool is_assignment_compatible(Type* other) override {
        if (Type::is_assignment_compatible(other)) {
            const auto* other_var = dynamic_cast<VariableType*>(other);
            return internal_type->is_assignment_compatible(
                other_var->internal_type.get());
        }

        return internal_type->is_assignment_compatible(other);
    }

    bool is_logical() override { return internal_type->is_logical(); }

    bool is_integral() override { return internal_type->is_integral(); }

    bool is_numeric() override { return internal_type->is_numeric(); }

    bool is_variable() override { return true; }
};

class VoidType final : public Type {};

class InvalidType final : public Type {
public:
    bool is_assignment_compatible(Type* /*other*/) override { return false; }
};
