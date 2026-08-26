#pragma once

#include "core/zctxt.h"
#include "ir/analysis/analyses.h"
#include "ir/analysis/verifier.h"
#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/pass.h"
#include <concepts>
#include <memory>
#include <print>
#include <string>
#include <utility>
#include <vector>

namespace z::ir {
template <std::invocable<const IRFunction&> Fn> class PassManager {
    std::vector<std::unique_ptr<IRPass>> fixpoint_passes;
    std::vector<std::unique_ptr<IRPass>> oneshot_passes;
    std::string print_before;
    std::string print_after;
    Fn dump;

public:
    PassManager(std::string print_before, std::string print_after, Fn&& dump)
        : print_before(std::move(print_before)),
          print_after(std::move(print_after)), dump(std::move(dump)) {}

    void add_fixpoint_pass(std::unique_ptr<IRPass> pass) {
        fixpoint_passes.push_back(std::move(pass));
    }

    void add_oneshot_pass(std::unique_ptr<IRPass> pass) {
        oneshot_passes.push_back(std::move(pass));
    }

    bool run_fixpoint_passes(IRFunction& func, ZContext& ctxt, bool debug) {
        bool changed = false;
        bool valid = true;
        auto analyses = FuncAnalyses(func, ctxt);

        while (true) {
            changed = false;
            for (const auto& pass : fixpoint_passes) {
                if (debug)
                    std::println("PassManager: running pass: {}", pass->name());

                if (print_before == pass->name()) {
                    std::println("before {}:", pass->name());
                    dump(func);
                }

                if (pass->run(func, analyses)) {
                    changed = true;
                    analyses.invalidate();
                }

                if (print_after == pass->name()) {
                    std::println("after {}:", pass->name());
                    dump(func);
                }

                if (debug) {
                    valid &= IRVerifier::verify(func);
                    if (!valid)
                        break;
                }
            }

            if (!changed)
                break;
        }

        return valid;
    }

    bool run_fixpoint_passes(IRFile& file, ZContext& ctxt, bool debug) {
        bool valid = true;

        for (auto& func : file.funcs) {
            valid &= run_fixpoint_passes(func, ctxt, debug);
        }

        return valid;
    }

    bool run_oneshot_passes(IRFunction& func, ZContext& ctxt, bool debug) {
        bool valid = true;
        bool out_of_ssa = false;
        auto analyses = FuncAnalyses(func, ctxt);

        for (const auto& pass : oneshot_passes) {
            if (debug)
                std::println("PassManager: running pass: {}", pass->name());

            if (print_before == pass->name())
                dump(func);

            pass->run(func, analyses);
            analyses.invalidate();

            if (pass->name() == "CopyLoweringPass")
                out_of_ssa = true;

            if (print_after == pass->name())
                dump(func);

            if (debug) {
                valid &= IRVerifier::verify(func);
                if (out_of_ssa)
                    valid &= IRVerifier::verify_out_of_ssa(func);
                if (!valid)
                    break;
            }
        }

        return valid;
    }

    bool run_oneshot_passes(IRFile& file, ZContext& ctxt, bool debug) {
        bool valid = true;

        for (auto& func : file.funcs) {
            valid &= run_oneshot_passes(func, ctxt, debug);
        }

        return valid;
    }
};
} // namespace z::ir
