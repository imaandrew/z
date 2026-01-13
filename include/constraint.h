#pragma once

#include "type.h"
#include "type_ref.h"
#include "zctxt.h"
#include <format>
#include <string>
#include <variant>

namespace z::type {

struct EqualityConstraint {
    TypeRef lhs;
    TypeRef rhs;

    [[nodiscard]] std::string to_string(ZContext* ctxt) const {
        return std::format("{} = {}", ctxt->ty->get(lhs)->basic_name(ctxt),
                           ctxt->ty->get(rhs)->basic_name(ctxt));
    }
};

using Constraint = std::variant<EqualityConstraint>;
} // namespace z::type
