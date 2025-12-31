#pragma once

#include "type.h"
#include <format>
#include <memory>
#include <string>
#include <variant>

namespace z::type {

struct EqualityConstraint {
    std::shared_ptr<Type> lhs;
    std::shared_ptr<Type> rhs;

    [[nodiscard]] std::string to_string() const {
        return std::format("{} = {}", lhs->basic_name(), rhs->basic_name());
    }
};

using Constraint = std::variant<EqualityConstraint>;
} // namespace z::type
