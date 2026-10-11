#pragma once

#include "core/index.h"
#include "core/panic.h"
#include "core/types.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace z::ir {
struct ConstTag;
using ConstIndex = z::Index<ConstTag>;
class ConstantPool {
    std::vector<u64> constants;
    std::unordered_map<u64, ConstIndex> index_map;

public:
    u16 intern(u64 val) {
        auto [it, inserted] =
            index_map.try_emplace(val, ConstIndex(constants.size()));
        if (inserted) {
            expect(constants.size() <= UINT16_MAX, "constant pool full");
            constants.push_back(val);
        }

        return it->second.id;
    }

    const std::vector<u64>& values() const { return constants; }
};
} // namespace z::ir
