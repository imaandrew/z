#pragma once

#include <vector>
#include <memory>

class Expr;
class Identifier;

class Type {};

class IntegerType : public Type {
    int size;
    bool _signed;

public:
    IntegerType(int size, bool _signed) : size(size), _signed(_signed) {};
};

class FloatType : public Type {
    int size;

public:
    FloatType(int size) : size(size) {};
};

class BooleanType : public Type {};

class StringType : public Type {};

class CharType : public Type {};

class PointerType : public Type {
    Type type;

public:
    PointerType(Type type) : type(type) {};
};

class ArrayType : public Type {
    Type type;
    std::unique_ptr<Expr> size;

public:
    ArrayType(Type type);
    ArrayType(Type type, std::unique_ptr<Expr> size);
    ~ArrayType();
};

class TupleType : public Type {
    std::vector<Type> types;

public:
    TupleType(std::vector<Type> types) : types(types) {};
};

class UserDefinedType : public Type {
    std::unique_ptr<Identifier> ident;

public:
    UserDefinedType(std::unique_ptr<Identifier> ident);
    ~UserDefinedType();
};
