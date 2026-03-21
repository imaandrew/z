#pragma once

#include "ir/ir.h"

namespace z::ir {
class IRPass {
public:
    IRPass() = default;
    virtual ~IRPass() = default;

    IRPass(const IRPass&) = delete;
    IRPass& operator=(const IRPass&) = delete;

    IRPass(IRPass&&) = delete;
    IRPass& operator=(IRPass&&) = default;

    [[nodiscard]] virtual const char* name() const = 0;
    virtual bool run(IRFunction& func) = 0;
};
} // namespace z::ir
