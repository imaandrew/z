#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace z {
class StringID {
    std::uint32_t id;
    explicit constexpr StringID(std::uint32_t id) : id(id) {}
    friend class StringPool;

public:
    bool operator==(const StringID& other) const { return id == other.id; }
    [[nodiscard]] std::uint32_t raw_id() const { return id; }
};
} // namespace z

namespace std {
template <> struct hash<z::StringID> {
    size_t operator()(const z::StringID& id) const {
        return hash<uint32_t>{}(id.raw_id());
    }
};
} // namespace std

namespace z {
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
