#pragma once

#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/pass.h"
#include <memory>
#include <utility>
#include <vector>

namespace z::ir {
class PassManager {
    std::vector<std::unique_ptr<IRPass>> passes;

public:
    void add_pass(std::unique_ptr<IRPass> pass) {
        passes.push_back(std::move(pass));
    }

    void run_passes(IRFunction& func) {
        bool changed = false;

        while (true) {
            changed = false;
            for (const auto& pass : passes) {
                if (pass->run(func)) {
                    changed = true;
                }
            }

            if (!changed)
                break;
        }
    }

    void run_passes(IRFile& file) {
        for (auto& func : file.funcs) {
            run_passes(func);
        }
    }
};
} // namespace z::ir
