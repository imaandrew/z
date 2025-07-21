#include "type.h"
#include "ast.h"
#include <memory>
#include <string>
#include <utility>

ArrayType::ArrayType(std::shared_ptr<Type> type) : type(std::move(type)) {};
ArrayType::ArrayType(std::shared_ptr<Type> type, std::shared_ptr<Expr> size)
    : type(std::move(type)), size(std::move(size)) {};
ArrayType::~ArrayType() = default;
UnknownType::UnknownType(std::unique_ptr<Identifier> ident)
    : ident(std::move(ident)) {};
UnknownType::~UnknownType() = default;

EnumType::EnumType(const std::unique_ptr<Identifier>& ident)
    : name(ident->to_string()) {};
StructType::StructType(const std::unique_ptr<Identifier>& ident)
    : name(ident->to_string()) {};

std::string UnknownType::to_string() const { return ident->to_string(); }

bool EnumType::is_assignment_compatible(const Type* other) const {
    if (Type::is_assignment_compatible(other)) {
        const auto* other_enum = dynamic_cast<const EnumType*>(other);
        return name == other_enum->name;
    }

    return false;
}

bool StructType::is_assignment_compatible(const Type* other) const {
    if (Type::is_assignment_compatible(other)) {
        const auto* other_struct = dynamic_cast<const StructType*>(other);
        return name == other_struct->name;
    }

    return false;
}