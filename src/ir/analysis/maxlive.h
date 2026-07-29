#pragma once

#include "core/types.h"
#include "ir/ir.h"
#include "liveness.h"

namespace z::ir {
u32 compute_maxlive(const IRFunction& func, const LivenessInfo& liveinfo);
}
