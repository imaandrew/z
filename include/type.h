#pragma once

#include <memory>
#include <utility>
#include <vector>

class Expr;
class Identifier;

class Type {
public:
    virtual ~Type() = default;
    Type() = default;

    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;
    Type(Type&&) = delete;
    Type& operator=(Type&&) = delete;
};

class IntegerType : public Type {
    int size;
    bool _signed;

public:
    IntegerType(int size, bool _signed) : size(size), _signed(_signed) {};
};

class FloatType : public Type {
    int size;

public:
    explicit FloatType(int size) : size(size) {};
};

class BooleanType : public Type {};

class StringType : public Type {};

class CharType : public Type {};

class PointerType : public Type {
    std::unique_ptr<Type> type;

public:
    explicit PointerType(std::unique_ptr<Type> type) : type(std::move(type)) {};
};

class ArrayType : public Type {
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
};

class TupleType : public Type {
    std::vector<Type> types;

public:
    explicit TupleType(std::vector<Type> types) : types(std::move(types)) {};
};

class UserDefinedType : public Type {
    std::unique_ptr<Identifier> ident;

public:
    explicit UserDefinedType(std::unique_ptr<Identifier> ident);
    ~UserDefinedType() override;

    UserDefinedType(const UserDefinedType&) = delete;
    UserDefinedType& operator=(const UserDefinedType&) = delete;
    UserDefinedType(UserDefinedType&&) = delete;
    UserDefinedType& operator=(UserDefinedType&&) = delete;
};
