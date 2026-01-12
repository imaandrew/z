#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

class StringID {
    std::uint32_t id;
    explicit constexpr StringID(std::uint32_t id) : id(id) {}
    friend class StringPool;

public:
    bool operator==(const StringID& other) const { return id == other.id; }
    [[nodiscard]] std::uint32_t raw_id() const { return id; }
};

template <> struct std::hash<StringID> {
    std::size_t operator()(const StringID& id) const {
        return std::hash<std::uint32_t>{}(id.raw_id());
    }
};

class StringPool {
    std::unordered_map<std::string_view, StringID> map;
    std::unordered_set<std::string> strings;
    std::uint32_t current_id = 0;

public:
    static constexpr StringID UNDERSCORE = StringID{0};

    StringPool() {
        auto [x, _] = strings.insert(std::string("_"));
        auto id = StringID(current_id++);
        map.insert({std::string_view(x->begin(), x->end()), id});
    }
    StringID intern(std::string_view s) {
        if (auto it = map.find(s); it != map.end()) {
            return it->second;
        }

        auto [x, _] = strings.insert(std::string(s));
        auto id = StringID(current_id++);
        map.insert({std::string_view(x->begin(), x->end()), id});
        return id;
    }
};