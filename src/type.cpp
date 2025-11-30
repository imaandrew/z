#include "type.h"
#include "ast.h"
#include <memory>
#include <string>
#include <utility>

ArrayType::ArrayType(std::shared_ptr<Type> type)
    : Type(Kind), type(std::move(type)) {};
ArrayType::ArrayType(std::shared_ptr<Type> type, std::shared_ptr<Expr> size)
    : Type(Kind), type(std::move(type)), size(std::move(size)) {};
ArrayType::~ArrayType() = default;
UnknownType::UnknownType(std::unique_ptr<Identifier> ident)
    : Type(Kind), ident(std::move(ident)) {};
UnknownType::~UnknownType() = default;

EnumType::EnumType(const std::unique_ptr<Identifier>& ident)
    : Type(Kind), name(ident->to_string()) {};
StructType::StructType(const std::unique_ptr<Identifier>& ident)
    : Type(Kind), name(ident->to_string()) {};

std::string UnknownType::to_string() const { return ident->to_string(); }
