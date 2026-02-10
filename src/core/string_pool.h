#pragma once

#include "core/index.h"
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>

namespace z {
struct StringTag {};
using StringID = Index<StringTag>;

class StringPool {
    std::unordered_map<std::string_view, StringID> map;
    std::deque<std::string> strings;
    std::uint32_t current_id = 0;

public:
    static constexpr StringID UNDERSCORE = StringID{0};

    StringPool() {
        strings.emplace_back("_");
        auto id = StringID(current_id++);
        map.insert({std::string_view(strings[id.id]), id});
    }

    StringID intern(std::string_view s) {
        if (auto it = map.find(s); it != map.end()) {
            return it->second;
        }

        strings.emplace_back(s);
        auto id = StringID(current_id++);
        map.insert({std::string_view(strings[id.id]), id});
        return id;
    }

    const std::string& get_string(StringID id) { return strings[id.id]; }
};
} // namespace z
