#pragma once

#include "core/types.h"
#include "ir/constants.h"
#include <optional>
#include <variant>
#include <vector>

namespace z {
namespace ast {
struct ConstDecl;
struct ArrayInitExpr;
struct StructInitExpr;
struct TupleExpr;
} // namespace ast

struct ConstValue;
struct ConstAggregate {
    std::vector<ConstValue> elems;
};

struct ConstValue {
    using T = std::variant<ir::ConstInt, ir::ConstFloat, bool, ConstAggregate>;
    T val;

    [[nodiscard]] ir::ConstInt* holds_int() {
        return std::get_if<ir::ConstInt>(&val);
    }

    [[nodiscard]] ir::ConstFloat* holds_float() {
        return std::get_if<ir::ConstFloat>(&val);
    }

    [[nodiscard]] bool* holds_bool() { return std::get_if<bool>(&val); }

    [[nodiscard]] ConstAggregate* holds_aggregate() {
        return std::get_if<ConstAggregate>(&val);
    }
};

enum class State : u8 { Unevaluated, InProgress, Done, Error };

struct ConstRecord {
    ast::ConstDecl* decl;
    State state = State::Unevaluated;
    std::optional<ConstValue> value;

    explicit ConstRecord(ast::ConstDecl* decl) : decl(decl) {}
};
} // namespace z
