#pragma once

#include "core/panic.h"
#include "core/zctxt.h"
#include "dom_tree.h"
#include "ir/ir.h"
#include "liveness.h"
#include "order.h"
#include "value_set.h"
#include <optional>

namespace z::ir {
class FuncAnalyses {
    IRFunction* func;
    std::optional<InstOrder> order_;
    std::optional<DominatorTree> dom_;
    std::optional<LivenessInfo> live_;
    ZContext* ctxt_;
    ValueSet values_;

public:
    explicit FuncAnalyses(IRFunction& func, ZContext& ctxt)
        : func(&func), ctxt_(&ctxt), values_(func.num_regs()) {}

    const InstOrder& order() {
        if (!order_)
            order_ = InstOrder(*func);
        return *order_;
    }

    const DominatorTree& dom() {
        if (!dom_)
            dom_ = DominatorTree::build(func->blocks, order());
        return *dom_;
    }

    const LivenessInfo& live() {
        if (!live_)
            live_ = LivenessBuilder(*func).build();
        return *live_;
    }

    ValueSet& values() {
        expect(values_.size() >= func->num_regs(),
               "ValueSet needs to be resized");
        return values_;
    }

    ZContext& ctxt() { return *ctxt_; }

    void invalidate() {
        order_.reset();
        dom_.reset();
        live_.reset();
    }
};
} // namespace z::ir
