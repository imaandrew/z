#include "type.h"
#include "ast.h"
#include <memory>
#include <string>

ArrayType::ArrayType(std::unique_ptr<Type> type) : type(std::move(type)) {};
ArrayType::ArrayType(std::unique_ptr<Type> type, std::unique_ptr<Expr> size)
    : type(std::move(type)), size(std::move(size)) {};
ArrayType::~ArrayType() = default;
UnknownType::UnknownType(std::unique_ptr<Identifier> ident)
    : ident(std::move(ident)) {};
UnknownType::~UnknownType() = default;

EnumType::EnumType(const std::unique_ptr<Identifier>& ident)
    : name(ident->to_string()) {};
StructType::StructType(const std::unique_ptr<Identifier>& ident)
    : name(ident->to_string()) {};

std::string UnknownType::to_string() { return ident->to_string(); }

bool EnumType::is_assignment_compatable(Type* other) {
    if (Type::is_assignment_compatable(other)) {
        auto* other_enum = dynamic_cast<EnumType*>(other);
        return name == other_enum->name;
    }

    return false;
}

bool StructType::is_assignment_compatable(Type* other) {
    if (Type::is_assignment_compatable(other)) {
        const auto* other_struct = dynamic_cast<StructType*>(other);
        return name == other_struct->name;
    }

    return false;
}