#include "type.h"
#include "ast.h"
#include <memory>
#include <utility>

namespace z::type {

ArrayType::ArrayType(std::shared_ptr<Type> type)
    : Type(Kind), type(std::move(type)) {};
ArrayType::ArrayType(std::shared_ptr<Type> type,
                     std::shared_ptr<ast::Expr> size)
    : Type(Kind), type(std::move(type)), size(std::move(size)) {};
ArrayType::~ArrayType() = default;

UnknownType::UnknownType(std::unique_ptr<ast::Identifier> ident)
    : Type(Kind), ident(std::move(ident)) {};
UnknownType::~UnknownType() = default;
bool UnknownType::operator==(const Type& other) const {
    if (const auto* other_ = dyn_cast<UnknownType>(&other)) {
        return ident->to_string() == other_->ident->to_string();
    }

    return false;
}

EnumType::EnumType(const std::unique_ptr<ast::Identifier>& ident)
    : Type(Kind), name(ident->to_string()) {};
StructType::StructType(const std::unique_ptr<ast::Identifier>& ident)
    : Type(Kind), name(ident->to_string()) {};

} // namespace z::type
