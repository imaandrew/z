#pragma once

#include "ir/analysis/analyses.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/pass.h"
#include <memory>
#include <utility>
#include <vector>

namespace z::ir {
class PassManager {
    std::vector<std::unique_ptr<IRPass>> fixpoint_passes;
    std::vector<std::unique_ptr<IRPass>> oneshot_passes;

public:
    void add_fixpoint_pass(std::unique_ptr<IRPass> pass) {
        fixpoint_passes.push_back(std::move(pass));
    }

    void add_oneshot_pass(std::unique_ptr<IRPass> pass) {
        oneshot_passes.push_back(std::move(pass));
    }

    void run_fixpoint_passes(IRFunction& func) {
        bool changed = false;
        auto analyses = FuncAnalyses(func);

        while (true) {
            changed = false;
            for (const auto& pass : fixpoint_passes) {
                if (pass->run(func, analyses)) {
                    changed = true;
                    analyses.invalidate();
                }
            }

            if (!changed)
                break;
        }
    }

    void run_fixpoint_passes(IRFile& file) {
        for (auto& func : file.funcs) {
            run_fixpoint_passes(func);
        }
    }

    void run_oneshot_passes(IRFunction& func) {
        auto analyses = FuncAnalyses(func);
        for (const auto& pass : oneshot_passes) {
            pass->run(func, analyses);
            analyses.invalidate();
        }
    }

    void run_oneshot_passes(IRFile& file) {
        for (auto& func : file.funcs) {
            run_oneshot_passes(func);
        }
    }
};
} // namespace z::ir
